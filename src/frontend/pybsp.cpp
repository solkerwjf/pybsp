#include "module.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s PYTHON_SCRIPT\n", argv[0]);
        return -1;
    }
    FILE *pyScript = fopen(argv[1],"r");
    if (pyScript == NULL) {
        fprintf(stderr, "ERROR: unable to open file '%s'\n", argv[1]);
        return -2;
    }

    initBSP(&argc, &argv);
    int err = PyRun_SimpleFileEx(pyScript,argv[1],1);
    if (err) {
        fprintf(stderr, "ERROR: failed to run file '%s' as a python script\n",
                argv[1]);
        return -3;
    }
    finiBSP();
    return 0;
}
