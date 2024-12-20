#include "compiler.h"
#include "exceptions/exceptions.h"
#include "utillities/util.h"

#include "parsing/bison/driver.h"
#include "parsing/bison/parser.hh"
#include "weeder/astweeder.h"

#include "environment-builder/environmentbuilder.h"
#include "type-linking/typelinker.h"
#include "hierarchy-checking/hierarchy-checking.h"
#include "disambiguation/forward-decl.h"
#include "disambiguation/search-unclassified.h"
#include "disambiguation/disambiguation.h"
#include "type-checker/typechecker.h"
#include "cfg-builder/cfg-builder.h"
#include "reachability/reachability.h"
#include "reachability/reached-statement.h"
#include "local-variables/local-variable-visitor.h"
#include "IR-builder/dispatch-vector.h"

#include "IR/ir.h"
#include "IR-builder/ast-to-ir.h"
#include "IR-interpreter/simulation/simulation.h"
#include "IR-canonicalizer/ir-canonicalizer.h"
#include "IR-canonicalizer/check-canonical.h"
#include "IR-interpreter/IR-java-converter/IR-java-converter.h"
#include "IR-tiling/tiling/ir-tiling.h"
#include "IR-tiling/register-allocation/brainless-allocator.h"
#include "IR/code-gen-constants.h"
#include "IR-tiling/assembly-generator/assembly-generator.h"

#include <regex>
#include <variant>

#ifdef GRAPHVIZ
    #include "graph/graph.h"
    #include "graph/ir_graph.h"
#endif

std::string Compiler::getEntryPointMethod() {
    if (infiles.size() < 1) {
        THROW_CompilerError("No first file for entrypoint");
    }

    if ( !strfiles.empty() ) {
        std::string class_name = "Foo";
        auto cls = Util::root_package->findClassDeclaration(class_name);
        auto entry_method = cls->all_methods["test"];
        if (!entry_method) {
            THROW_CompilerError("Class '" + class_name + "' has no method 'test'");
        }
        return CGConstants::uniqueStaticMethodLabel(entry_method);
    }

    std::string first_file = infiles.front();
    std::string base_file = first_file.substr(first_file.find_last_of("/") + 1);
    std::string class_name = std::regex_replace(base_file, std::regex(".java"), "");

    auto cls = Util::root_package->findClassDeclaration(class_name);
    if (!cls) {
        THROW_CompilerError("First file '" + first_file + "' does not have class '" + class_name + "'");
    }

    auto entry_method = cls->all_methods["test"];
    if (!entry_method) {
        THROW_CompilerError("Class '" + class_name + "' has no method 'test'");
    }

    return CGConstants::uniqueStaticMethodLabel(entry_method);
}

int Compiler::finishWith(ReturnCode code) {
    if ( output_rc ) { cerr << "RETURN CODE " << code << endl; }
    return code;
}

struct Destructor {
    ~Destructor() {
        AddLocation::deleteFileNames();
    }
};

int Compiler::run() {
    Destructor destruct;
    Driver drv;
    AstWeeder weeder;
    vector<AstNodeVariant> asts;
    Util::linked_asts = &asts;
    
    // Lexing and parsing
    try {
        drv.trace_scanning = trace_scanning;
        drv.trace_parsing = trace_parsing;

        std::list<std::string> files = infiles;

        for ( auto it = strfiles.rbegin(); it != strfiles.rend(); it++ ) {
            std::string &strfile = *it;

            // Add in reverse to both list's front
            drv.strfiles.push_front(strfile);
            files.push_front("Foo.java");   // dummy filename for strfile
        }

        for (auto file : files) {
            bool is_strfile = !drv.strfiles.empty();
            std::string strfile = is_strfile ? drv.strfiles.front() : "";
            int rc = drv.parse(file);

            if (rc != 0) {
                cerr << "Parsing failed" << endl;
                return finishWith(ReturnCode::INVALID_PROGRAM);
            }

            AstNodeVariant ast = std::move(*drv.root);            

            rc = weeder.weed(ast, file, strfile);

            if (rc != 0) {
                cerr << "Weeding failed" << endl;
                return finishWith(ReturnCode::INVALID_PROGRAM);
            }

            asts.emplace_back(std::move(ast));
        }

    } catch ( ... ) {
        cerr << "Unknown exception occured in syntactic analysis" << endl;
        return finishWith(ReturnCode::INVALID_PROGRAM);
    }

    PackageDeclarationObject default_package;
    Util::root_package = &default_package;

    try {
        #ifdef GRAPHVIZ
            GraphVisitor gv(asts); // runs on return/destruct
        #endif

        // Environment building
        for (auto &ast : asts) {
            EnvironmentBuilder(default_package).visit(ast);
        }

        // Type linking
        for (auto& ast : asts) {
            TypeLinker(default_package).visit(ast);
        } 

        // Hierarchy checking
        for ( auto& ast : asts ) {
            HierarchyCheckingVisitor(default_package).visit(ast);
        }

        // Disambiguation of names
        for (auto &ast: asts ) {
            DisambiguationVisitor(default_package).visit(ast);
        }

        // Disambiguation of names (forward decl)
        for (auto &ast: asts ) {
            ForwardDeclarationVisitor(default_package).visit(ast);
        }

        // Check for unclassified identifiers (optional)
        for (auto &ast: asts ) {
            SearchUnclassifiedVisitor().visit(ast);
        }

        // Type checking
        for (auto &ast: asts ) {
            TypeChecker(default_package).visit(ast);
        }

        // CfgBuilder
        for (auto &ast: asts) {
            CfgBuilderVisitor().visit(ast);
        } 

        // Reachability testing
        for (auto &ast : asts) {
            CfgReachabilityVisitor().visit(ast);
        }

        // Reachability testing
        for (auto &ast : asts) {
            StatementVisitor().visit(ast);
        }

        // Local variable checking
        for (auto &ast: asts) {
            LocalVariableVisitor().visit(ast);
        }

        // Dispatch vector creation
        for (auto &ast: asts) {
            DVBuilder().visit(ast);
        }
        DVBuilder::assignColours();
        DVBuilder::assertColoured();

        for (auto &ast: asts) {
            // PrintDVS().visit(ast);
        }

        // Code generation
        if (emit_code) {
            auto entrypoint_method = getEntryPointMethod();

            // Convert to IR
            std::vector<IR> IR_asts;
            for (auto &ast: asts) {
                IR_asts.emplace_back(IRBuilderVisitor().visit(ast));
            }

            #ifdef GRAPHVIZ
                // Graph IR
                IRGraphVisitor().visit(IR_asts.front());
            #endif

            if (run_java_ir) {
                // Convert to Java IR
                IRJavaConverter converter = IRJavaConverter("MainNonCanonical", entrypoint_method);
                converter.visit(IR_asts.front());
                std::string result = converter.getResult();
                std::ofstream java_file {"java-ir/ir/interpret/MainNonCanonical.java"};
                java_file << result;
            } else if (run_ir) {
                // Run interpreter on IR and get value
                try {
                    auto sim = Simulator(&IR_asts.front());
                    sim.setDebugLevel(0);
                    int result = sim.call(entrypoint_method, {});
                    
                    std::ofstream result_file {"ir_result.tmp"};
                    result_file << result;
                } catch (const SimulatorError &e ) {
                    cerr << e.what() << "\n";
                    #ifdef LIBFUZZER
                        if ( !e.soft ) {
                            THROW_LibfuzzerError();
                        }
                    #endif
                }
            }

             // Canonicalize IR
            for (auto &ir_ast: IR_asts) {
                IRCanonicalizer().convert(ir_ast);
                if (!CanonicalChecker().check(ir_ast, true)) {
                    THROW_CompilerError("IR is not canonical after canonicalizing");
                }
            }

            #ifndef LIBFUZZER
                if (run_java_ir) {
                    IRJavaConverter converter = IRJavaConverter("MainCanonical", entrypoint_method);
                    converter.visit(IR_asts.front());
                    std::string result = converter.getResult();
                    std::ofstream java_file {"java-ir/ir/interpret/MainCanonical.java"};
                    java_file << result;
                } else if (run_ir) {
                    // Run interpreter on Canonical IR and get value
                    try {
                        auto sim = Simulator(&IR_asts.front());
                        sim.setDebugLevel(0);
                        int result = sim.call(entrypoint_method, {});

                        std::ofstream result_file {"ir_canon_result.tmp"};
                        result_file << result;
                    } catch (const SimulatorError &e ) {
                        cerr << e.what() << "\n";
                    }
                }
            #endif

            if (optimization == OptimizationType::UNOPTIMIZED) {
                AssemblyGenerator().generateCode(IR_asts, entrypoint_method, "brainless");
            } else if (optimization == OptimizationType::REGISTER_ALLOCATION) {
                AssemblyGenerator().generateCode(IR_asts, entrypoint_method, "linear-scan");
            } else {
                THROW_CompilerError("Unknown optimization type");
            }
        }

    } catch (const CompilerError &e ) {
        cerr << e.what() << "\n";
        return finishWith(ReturnCode::COMPILER_DEVELOPMENT_ERROR);
    } catch (const std::exception &e) {
        cerr << e.what() << "\n";
        return finishWith(ReturnCode::INVALID_PROGRAM);
    } catch (...) {
        cerr << "Unknown error occurred\n";
        return finishWith(ReturnCode::COMPILER_DEVELOPMENT_ERROR);
    }

    return finishWith(ReturnCode::VALID_PROGRAM);
}
