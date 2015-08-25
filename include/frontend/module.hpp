#ifndef __PCF_PY_MODULE_HPP__
#define __PCF_PY_MODULE_HPP__

#include <python.h>
extern "C" {
    void initBSP(int *pArgc, char ***pArgv);
    void finiBSP();
}

#endif

