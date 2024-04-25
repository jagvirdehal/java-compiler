#include <unistd.h>
#include <iostream>
#include <exception>
#include <sstream>
#include "compiler/compiler.h"
#include <getopt.h>

using namespace std;

enum class CommandLineArg {
    OUTPUT_RETURN = 'r',
    TRACE_PARSING = 'p',
    TRACE_SCANNING = 's',
    STATIC_ANALYSIS_ONLY = 'a', // Don't emit IR/assembly; used for pre-A5 tests
    RUN_AND_TEST_IR = 'i',
    RUN_AND_TEST_JAVA_IR = 'j',
    NO_OPTIMIZATION = 'o',
    OPTIMIZED = 'b'
};


struct cmd_error {};


int main(int argc, char *argv[]) {
    Compiler compiler;

    const struct option longopts[] = {
        { "output-return", no_argument, 0, 'r'},
        { "trace-parsing", no_argument, 0, 'p'},
        { "trace-scanning", no_argument, 0, 's'},
        { "static-analysis", no_argument, 0, 'a'},
        { "run-ir", no_argument, 0, 'i'},
        { "run-java-ir", no_argument, 0, 'j'},
        { "opt-none", no_argument, 0, 'o'},
        { "optimized", required_argument, 0, 'b'},
        {0, 0, 0, 0}
    };

    int index;
    char c = 0;

    try {
        // Handle optional arguments (eg. enable parse debugging)
        while (true) {
            c = getopt_long(argc, argv, "rpsaijob:", longopts, &index);

            if ( c == -1 ) break;

            switch (c) {
                case 'r':
                    compiler.setOutputRC(true);
                    break;
                case 'p':
                    compiler.setTraceParsing(true);
                    break;
                case 's':
                    compiler.setTraceScanning(true);
                    break;
                case 'a':
                    compiler.setEmitCode(false);
                    break;
                case 'i':
                    compiler.setRunIR(true);
                    break;
                case 'j':
                    compiler.setRunJavaIR(true);
                    break;
                case 'o':
                    compiler.setOptimizationType(Compiler::OptimizationType::UNOPTIMIZED);
                    std::cout << "compiled without optimization" << std::endl;
                    break;
                case 'b':
                {
                    std::string arg = std::string(optarg);
                    if (arg == "opt-reg-only") {
                        std::cout << "compiled with reg alloc optimization" << std::endl;
                        compiler.setOptimizationType(Compiler::OptimizationType::REGISTER_ALLOCATION);
                    }
                    break;
                }
                default:
                    throw cmd_error();
            }
        }

        // For each non-option argument, add to infiles
        for (int i = optind; i < argc; i++) {
            // Check that the file exists
            if (access(argv[i], F_OK) == -1) {
                cerr << "File " << argv[i] << " does not exist" << endl;
                return compiler.finishWith(Compiler::USAGE_ERROR);
            }

            compiler.addInFile(argv[i]);
        }

        if (compiler.inFilesEmpty()) {
            throw cmd_error();
        }
    } catch ( cmd_error & e ) {
        cerr 
            << "Usage:\n\t"
            << argv[0]
            << " <filename>"
            << " [ --output-return [-r] \n\t\t--trace-parsing [-p] \n\t\t--trace-scanning [-s] \n\t\t--static-analysis [-a] \n\t\t--run-ir [-i] \n\t\t--run-java-ir [-j] \n\t\t--opt-none [-o] \n\t\t--optimized [-b] (opt-reg-only)]"
            << "\n";
        return compiler.finishWith(Compiler::USAGE_ERROR);
    } catch ( ... ) {
        cerr << "Error occured on input\n";
        return compiler.finishWith(Compiler::USAGE_ERROR);
    }

    return compiler.run();
}
