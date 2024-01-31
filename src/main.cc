#include <unistd.h>
#include <cstring>
#include <iostream>
#include "parsing/bison/location.hh"
#include "parsing/parseuser.h"
#include "parsing/bison/driver.h"
#include "parsing/bison/parser.hh"
using namespace std;

enum return_codes {
    PARSING_SUCCESS = 0,
    PARSING_FAILURE = 42,
};

struct cmd_error {};

int main(int argc, char *argv[]) {
    string infile;
    bool trace_parsing = false;
    bool trace_scanning = false;

    try {
        // Handle optional arguments (eg. enable parse debugging)
        int opt;
        while ((opt = getopt(argc, argv, "ps")) != -1) {
            switch (opt) {
                case 'p':
                    trace_parsing = true;
                    break;
                case 's':
                    trace_scanning = true;
                    break;
                default:
                    throw cmd_error();
            }
        }

        switch ( argc - optind ) {
            case 1:
                infile = argv[optind];
                break;
            default:
                throw cmd_error();
        }
    } catch ( cmd_error & e ) {
        cerr << "Usage:\n\t"
            << argv[0]
            << " <filename>"
            << " [ -p -s ]"
            << endl;
        return EXIT_FAILURE;
    } catch ( ... ) {
        cerr << "Error occured on input" << endl;
        return EXIT_FAILURE;
    }

    int rc = 0;
    Driver drv;

    try {
        drv.trace_parsing = trace_parsing;
        rc = drv.parse(infile);
    } catch ( ... ) {
        cerr << "Exception occured" << endl;
        return PARSING_FAILURE;
    }

    int parsing_rc = ((rc == 0) ? PARSING_SUCCESS : PARSING_FAILURE);

    cerr << "RETURN CODE " << parsing_rc << endl;
    return parsing_rc;
}
