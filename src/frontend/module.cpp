#include "module.hpp"
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include "BSP.hpp"
#include <cassert>
#include <string>
#include <sstream>
#include <map>

using namespace BSP;
PyObject *cPickle_ = NULL;
PyObject *cPickle_dumps_ = NULL;
PyObject *cPickle_loads_ = NULL;
PyObject *traceback_ = NULL;
PyObject *traceback_extractStack_ = NULL;
PyObject **fromProc_ = NULL;
uint64_t nProcs_ = 0;
std::map<IndexSet *, int> indexSetToID_;
std::map<int, IndexSet *> idToIndexSet_;
IndexSet *activeIndexSet_ = NULL;

Runtime *runtime_ = NULL;

std::string bsp_getScriptPos() {
    std::stringstream ss;
    ss << "------ call stack begin ------ " << std::endl;
    PyObject *param = Py_BuildValue("()");
    PyObject *stack = PyObject_CallObject(traceback_extractStack_,param);
    Py_DECREF(param);
    Py_ssize_t stackSize = PyList_GET_SIZE(stack);
    for (Py_ssize_t level = 0; level < stackSize; ++level) {
        PyObject *frame = PyList_GET_ITEM(stack, stackSize - 1 - level);
        char *fileName = NULL;
        long line = 0;
        char *funcName = NULL;
        char *lineText = NULL;
        int ok = PyArg_ParseTuple(frame,"slss:bsp.getScriptPos", &fileName,&line,&funcName,&lineText);
        if (!ok) {
            PyErr_SetString(PyExc_RuntimeError, "error occured when parsing stack frames in bsp.getScriptPos");
            return "";
        }
        ss << "#" << level << ": FILE:" << fileName << ", LINE:" << line << ", FUNCTION:" << funcName << ", CODE:" <<std::endl
            << ">>> " << lineText << std::endl;
    }
    ss << "------  call stack end  ------ " << std::endl;
    return ss.str();
}

void bsp_typeError(std::string strErr) {
    PyErr_SetString(PyExc_TypeError, (strErr + "\n" + bsp_getScriptPos()).c_str());
    PyErr_Print();
}

void bsp_runtimeError(std::string strErr) {
    PyErr_SetString(PyExc_RuntimeError, (strErr + "\n" + bsp_getScriptPos()).c_str());
    PyErr_Print();
}

extern "C" {
    void finiBSP() {
        for (std::map<IndexSet *, int>::iterator iter = indexSetToID_.begin();
                iter != indexSetToID_.end(); ++iter) {
            delete iter->first;
        }
        indexSetToID_.clear();
        idToIndexSet_.clear();
        activeIndexSet_ = NULL;
        Py_XDECREF(cPickle_dumps_);
        Py_XDECREF(cPickle_loads_);
        Py_XDECREF(cPickle_);
        Py_XDECREF(traceback_extractStack_);
        Py_XDECREF(traceback_);
        for (unsigned i = 0; i < nProcs_; ++i) {
            Py_XDECREF(fromProc_[i]);
        }
        delete[] fromProc_;
        cPickle_ = NULL;
        cPickle_dumps_ = NULL;
        cPickle_loads_ = NULL;
        fromProc_ = NULL;
        nProcs_ = 0;
        Py_Finalize();
        delete runtime_;
    }

    // myProcID = bsp.myProcID()
    static PyObject *bsp_myProcID(PyObject *self, PyObject *args) {
        int ok = PyArg_ParseTuple(args,":bsp.myProcID");
        if (!ok) {
            bsp_typeError("bsp.myProcID requires no arguments");
            Py_RETURN_NONE;
        }
        uint64_t result = runtime_->getMyProcessID();
        return Py_BuildValue("l",(long)result);
    }

    // procCount = bsp.procCount()
    static PyObject *bsp_procCount(PyObject *self, PyObject *args) {
        int ok = PyArg_ParseTuple(args,":bsp.procCount");
        if (!ok) {
            bsp_typeError("bsp.procCount requires no arguments");
            Py_RETURN_NONE;
        }
        return Py_BuildValue("l",(long)nProcs_);
    }

    // importedFromProcID = bsp.fromProc(procID)
    static PyObject *bsp_fromProc(PyObject *self, PyObject *args) {
        long procID;
        int ok = PyArg_ParseTuple(args,"l:bsp.fromProc", &procID);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.fromProc(procID)");
            Py_RETURN_NONE;
        }
        if (NULL == fromProc_) {
            return Py_BuildValue("{}");
        } else {
            Py_XINCREF(fromProc_[procID]);
            return fromProc_[procID];
        }
    }

    // OK = bsp.fromObject(object,arrayPath)
    static PyObject *bsp_fromObject(PyObject *self, PyObject *args) {
        PyObject *input = NULL;
        char *path = NULL;
        int ok = PyArg_ParseTuple(args,"Os:bsp.fromObject",&input,&path);
        Py_XINCREF(input);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.fromObject(object,arrayPath)");
            Py_XDECREF(input);
            return Py_BuildValue("O",Py_False);
        }
        PyObject *param = Py_BuildValue("(O)",input);
        PyObject *strObj = PyObject_CallObject(cPickle_dumps_,param);
        Py_XDECREF(param);
        Py_XDECREF(input);
        if (NULL == strObj) {
            bsp_runtimeError("failed to call cPickle.dumps()");
            return Py_BuildValue("O",Py_False);
        }
        char *str = NULL;
        ok = PyArg_Parse(strObj,"s:bsp.fromObject.getStr",&str);
        Py_DECREF(strObj);
        if (!ok) {
            bsp_runtimeError("failed to get string from output of cPickle.dumps()");
            return Py_BuildValue("O",Py_False);
        }
        try {
            runtime_->fromString(std::string(str),std::string(path));
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.fromNumpy(numpyArray,arrayPath)
    static PyObject *bsp_fromNumpy(PyObject *self, PyObject *args) {
        PyObject *input = NULL;
        char *path = NULL;
        int ok = PyArg_ParseTuple(args,"Os:bsp.fromNumpy",&input,&path);
        Py_XINCREF(input);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.fromNumpy(NumpyArray,arrayPath)");
            Py_XDECREF(input);
            return Py_BuildValue("O",Py_False);
        }
        if (!PyArray_Check(input)) {
            bsp_typeError("invalid arguments for bsp.fromNumpy(NumpyArray,arrayPath)");
            Py_XDECREF(input);
            return Py_BuildValue("O",Py_False);
        }
        PyArrayObject *numpyArray = (PyArrayObject *)input;
        int nDims = PyArray_NDIM(numpyArray);
        npy_intp *dimSize = PyArray_DIMS(numpyArray);
        npy_intp *strides = PyArray_STRIDES(numpyArray);
        char *data = PyArray_BYTES(numpyArray);
        char kind = 'i';
        if (PyArray_ISINTEGER(numpyArray)) {
            if (PyArray_ISUNSIGNED(numpyArray))
                kind = 'u';
            else
                kind = 'i';
        } else if (PyArray_ISCOMPLEX(numpyArray)) {
            kind = 'c';
        } else if (PyArray_ISFLOAT(numpyArray)) {
            kind = 'f';
        }
        try {
            runtime_->fromBuffer(data, kind, nDims, dimSize, strides, path);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            Py_XDECREF(input);
            return Py_BuildValue("O",Py_False);
        }
        Py_XDECREF(input);
        return Py_BuildValue("O",Py_True);
    }

    // object = bsp.toObject(arrayPath)
    static PyObject *bsp_toObject(PyObject *self, PyObject *args) {
        PyObject *output = NULL;
        char *path = NULL;
        int ok = PyArg_ParseTuple(args,"s:bsp.toObject",&path);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.toObject(arrayPath)");
            Py_RETURN_NONE;
        }
        try {
            std::string strOutput = runtime_->toString(std::string(path));
            PyObject *objStr = Py_BuildValue("(s)",strOutput.c_str());
            output = PyObject_CallObject(cPickle_loads_,objStr);
            Py_XDECREF(objStr);
            if (NULL == output) {
                bsp_runtimeError("failed to call cPickle.loads()");
                Py_RETURN_NONE;
            }
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            Py_RETURN_NONE;
        }
        return output;
    }

    // numpyArray = bsp.toNumpy(arrayPath)
    static PyObject *bsp_toNumpy(PyObject *self, PyObject *args) {
        PyObject *output = NULL;
        char *path = NULL;
        int ok = PyArg_ParseTuple(args,"s:bsp.toNumpy",&path);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.toNumpy(arrayPath)");
            Py_RETURN_NONE;
        }
        try {
            NamedObject *nobj = runtime_->getObject(std::string(path));
            LocalArray *localArray = nobj->_localArray();
            ArrayShape::ElementType elemType = localArray->getElementType();
            int typeNum = NPY_DOUBLE;
            switch (elemType) {
                case ArrayShape::INT8:
                    typeNum = NPY_INT8;
                    break;
                case ArrayShape::INT16:
                    typeNum = NPY_INT16;
                    break;
                case ArrayShape::INT32:
                    typeNum = NPY_INT32;
                    break;
                case ArrayShape::INT64:
                    typeNum = NPY_INT64;
                    break;
                case ArrayShape::UINT8:
                    typeNum = NPY_UINT8;
                    break;
                case ArrayShape::UINT16:
                    typeNum = NPY_UINT16;
                    break;
                case ArrayShape::UINT32:
                    typeNum = NPY_UINT32;
                    break;
                case ArrayShape::UINT64:
                    typeNum = NPY_UINT64;
                    break;
                case ArrayShape::FLOAT:
                    typeNum = NPY_FLOAT;
                    break;
                case ArrayShape::DOUBLE:
                    typeNum = NPY_DOUBLE;
                    break;
                case ArrayShape::CFLOAT:
                    typeNum = NPY_CFLOAT;
                    break;
                case ArrayShape::CDOUBLE:
                    typeNum = NPY_CDOUBLE;
                    break;
                default:
                    break;
            }
            int nDims = (int) localArray->getNumberOfDimensions();
            npy_intp dimSize[7];
            for (int iDim = 0; iDim < nDims; ++iDim) {
                dimSize[iDim] = (npy_intp) localArray->getElementCount(iDim);
            }
            output = PyArray_SimpleNew(nDims,dimSize,typeNum);
            if (!PyArray_Check(output)) {
                bsp_runtimeError("failed to call PyArray_SimpleNew() in bsp.toNumpy(arrayPath)");
                Py_XDECREF(output);
                Py_RETURN_NONE;
            }
            char *numpyData = (char *)PyArray_DATA((PyArrayObject *)output);
            memcpy(numpyData, localArray->getData(), localArray->getByteCount());
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            Py_XDECREF(output);
            Py_RETURN_NONE;
        }
        return output;
    }

    // numpyArray = bsp.asNumpy(arrayPath)
    static PyObject *bsp_asNumpy(PyObject *self, PyObject *args) {
        PyObject *output = NULL;
        char *path = NULL;
        int ok = PyArg_ParseTuple(args,"s:bsp.asNumpy",&path);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.asNumpy(arrayPath)");
            Py_RETURN_NONE;
        }
        try {
            NamedObject *nobj = runtime_->getObject(std::string(path));
            LocalArray *localArray = nobj->_localArray();
            ArrayShape::ElementType elemType = localArray->getElementType();
            int typeNum = NPY_DOUBLE;
            switch (elemType) {
                case ArrayShape::INT8:
                    typeNum = NPY_INT8;
                    break;
                case ArrayShape::INT16:
                    typeNum = NPY_INT16;
                    break;
                case ArrayShape::INT32:
                    typeNum = NPY_INT32;
                    break;
                case ArrayShape::INT64:
                    typeNum = NPY_INT64;
                    break;
                case ArrayShape::UINT8:
                    typeNum = NPY_UINT8;
                    break;
                case ArrayShape::UINT16:
                    typeNum = NPY_UINT16;
                    break;
                case ArrayShape::UINT32:
                    typeNum = NPY_UINT32;
                    break;
                case ArrayShape::UINT64:
                    typeNum = NPY_UINT64;
                    break;
                case ArrayShape::FLOAT:
                    typeNum = NPY_FLOAT;
                    break;
                case ArrayShape::DOUBLE:
                    typeNum = NPY_DOUBLE;
                    break;
                case ArrayShape::CFLOAT:
                    typeNum = NPY_CFLOAT;
                    break;
                case ArrayShape::CDOUBLE:
                    typeNum = NPY_CDOUBLE;
                    break;
                default:
                    break;
            }
            int nDims = (int) localArray->getNumberOfDimensions();
            npy_intp dimSize[7];
            for (int iDim = 0; iDim < nDims; ++iDim) {
                dimSize[iDim] = (npy_intp) localArray->getElementCount(iDim);
            }
            output = PyArray_SimpleNewFromData(nDims,dimSize,typeNum, localArray->getData());
            if (!PyArray_Check(output)) {
                bsp_runtimeError("failed to call PyArray_SimpleNewWithData() in bsp.toNumpy(arrayPath)");
                Py_XDECREF(output);
                Py_RETURN_NONE;
            }
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            Py_XDECREF(output);
            Py_RETURN_NONE;
        }
        return output;
    }

    // OK = bsp.createArray(arrayPath,dtype,arrayShape)
    static PyObject *bsp_createArray(PyObject *self, PyObject *args) {
        char *arrayPath = NULL;
        char *dtype = NULL;
        PyObject *arrayShape = NULL;
        uint64_t dimSize[7] = {0,0,0,0,0,0,0};
        int ok = PyArg_ParseTuple(args,"ssO:bsp.createArray",&arrayPath,&dtype,&arrayShape);
        Py_XINCREF(arrayShape);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.createArray(arrayPath,dtype,arrayShape)");
            Py_XDECREF(arrayShape);
            return Py_BuildValue("O",Py_False);
        }
        if (PyTuple_Check(arrayShape)) {
            ok = PyArg_ParseTuple(arrayShape,"k|kkkkkk:bsp.createArray.extractArrayShape",
                    (unsigned long *)(dimSize + 0),
                    (unsigned long *)(dimSize + 1),
                    (unsigned long *)(dimSize + 2),
                    (unsigned long *)(dimSize + 3),
                    (unsigned long *)(dimSize + 4),
                    (unsigned long *)(dimSize + 5),
                    (unsigned long *)(dimSize + 6));
            Py_XDECREF(arrayShape);
            if (!ok || dimSize[0] == 0) {
                bsp_typeError("invalid array shape for bsp.createArray(arrayPath,dtype,arrayShape)");
                return Py_BuildValue("O",Py_False);
            }
        } else if (PyList_Check(arrayShape)) {
            Py_ssize_t nItems = PyList_GET_SIZE(arrayShape);
            for (Py_ssize_t iItem = 0; iItem < nItems; ++iItem) {
                PyObject *item = PyList_GET_ITEM(arrayShape,iItem);
                Py_XINCREF(item);
                unsigned long sizeOfThisDim = 0;
                ok = PyArg_Parse(item, "k", &sizeOfThisDim);
                Py_XDECREF(item);
                if (!ok)
                    break;
                dimSize[iItem] = sizeOfThisDim;
            }
            Py_XDECREF(arrayShape);
            if (!ok || dimSize[0] == 0) {
                bsp_typeError("invalid array shape for bsp.createArray(arrayPath,dtype,arrayShape)");
                return Py_BuildValue("O",Py_False);
            }
        } else {
            Py_XDECREF(arrayShape);
            bsp_typeError("invalid array shape for bsp.createArray(arrayPath,dtype,arrayShape)");
            return Py_BuildValue("O",Py_False);
        }

        unsigned nDims = 1;
        for (unsigned iDim = 1; iDim < 7; ++iDim) {
            if (dimSize[iDim] == 0) {
                break;
            } 
            ++ nDims;
        }

        ArrayShape::ElementType elemType = ArrayShape::BINARY;
        if (0 == strcmp(dtype,"i1") || 0 == strcmp(dtype,"int8"))
            elemType = ArrayShape::INT8;
        else if (0 == strcmp(dtype,"i2") || 0 == strcmp(dtype,"int16"))
            elemType = ArrayShape::INT16;
        else if (0 == strcmp(dtype,"i4") || 0 == strcmp(dtype,"int32"))
            elemType = ArrayShape::INT32;
        else if (0 == strcmp(dtype,"i8") || 0 == strcmp(dtype,"int64"))
            elemType = ArrayShape::INT64;
        else if (0 == strcmp(dtype,"u1") || 0 == strcmp(dtype,"uint8"))
            elemType = ArrayShape::UINT8;
        else if (0 == strcmp(dtype,"u2") || 0 == strcmp(dtype,"uint16"))
            elemType = ArrayShape::UINT16;
        else if (0 == strcmp(dtype,"u4") || 0 == strcmp(dtype,"uint32"))
            elemType = ArrayShape::UINT32;
        else if (0 == strcmp(dtype,"u8") || 0 == strcmp(dtype,"uint64"))
            elemType = ArrayShape::UINT64;
        else if (0 == strcmp(dtype,"f") || 0 == strcmp(dtype,"f4") || 0 == strcmp(dtype,"float32"))
            elemType = ArrayShape::FLOAT;
        else if (0 == strcmp(dtype,"d") || 0 == strcmp(dtype,"f8") || 0 == strcmp(dtype,"float64"))
            elemType = ArrayShape::DOUBLE;
        else if (0 == strcmp(dtype,"c8") || 0 == strcmp(dtype,"complex64"))
            elemType = ArrayShape::CFLOAT;
        else if (0 == strcmp(dtype,"c") || 0 == strcmp(dtype,"c16") || 0 == strcmp(dtype,"complex128"))
            elemType = ArrayShape::CDOUBLE;
        if (elemType == ArrayShape::BINARY) {
            bsp_typeError("invalid dtype for bsp.createArray(arrayPath,dtype,arrayShape)");
            return Py_BuildValue("O",Py_False);
        }
        uint64_t elemSize = ArrayShape::elementSize(elemType);
        try {
            LocalArray *localArray = new LocalArray(std::string(arrayPath), elemType, elemSize, nDims, dimSize);
            if (localArray == NULL) {
                bsp_runtimeError("failed to call bsp.createArray(arrayPath,dtype,arrayShape)");
                return Py_BuildValue("O",Py_False);
            }
            runtime_->setObject(std::string(arrayPath), localArray);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.delete(up-to-10-paths)
    static PyObject *bsp_delete(PyObject *self, PyObject *args) {
        char *path[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
        int ok = PyArg_ParseTuple(args, "s|sssssssss:bsp.delete",
                path + 0,
                path + 1,
                path + 2,
                path + 3,
                path + 4,
                path + 5,
                path + 6,
                path + 7,
                path + 8,
                path + 9);
        if (!ok) {
            bsp_typeError("invalid dtype for bsp.delete(up-to-10-paths)");
            return Py_BuildValue("O",Py_False);
        }
        for (int i = 0; i < 10; ++i) {
            if (NULL == path[i])
                break;
            try {
                runtime_->deleteObject(std::string(path[i]));
            } catch (const std::exception &e) {
                std::stringstream ssErr;
                ssErr << "failed to delete " << path[i] << ": " << e.what();
                bsp_runtimeError(ssErr.str());
                return Py_BuildValue("O",Py_False);
            }
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.share(up-to-10-array-Paths)
    static PyObject *bsp_share(PyObject *self, PyObject *args) {
        char *path[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
        int ok = PyArg_ParseTuple(args, "s|sssssssss:bsp.share",
                path + 0,
                path + 1,
                path + 2,
                path + 3,
                path + 4,
                path + 5,
                path + 6,
                path + 7,
                path + 8,
                path + 9);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.share(up-to-10-paths)");
            return Py_BuildValue("O",Py_False);
        }
        std::vector<std::string> arrayPaths;
        for (int i = 0; i < 10; ++i) {
            if (NULL == path[i])
                break;
            arrayPaths.push_back(std::string(path[i]));
        }
        try {
            runtime_->share(arrayPaths);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.globalize(procStart,gridShape,up-to-10-array-Paths)
    static PyObject *bsp_globalize(PyObject *self, PyObject *args) {
        uint64_t procStart = 0;
        PyObject *objGridShape = NULL;
        uint64_t gridDimSize[7] = {0,0,0,0,0,0,0};
        unsigned nGridDims = 0;
        char *path[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
        int ok = PyArg_ParseTuple(args,"kOs|sssssssss:bsp.globalize",
                (unsigned long *)&procStart,
                &objGridShape,
                path + 0,
                path + 1,
                path + 2,
                path + 3,
                path + 4,
                path + 5,
                path + 6,
                path + 7,
                path + 8,
                path + 9);
        Py_XINCREF(objGridShape);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.globalize(procStart,gridShape,up-to-10-array-paths)");
            Py_XDECREF(objGridShape);
            return Py_BuildValue("O",Py_False);
        }
        ok = PyArg_ParseTuple(objGridShape,"k|kkkkkk:bsp.globalize.extractGridShape",
                (unsigned long *)(gridDimSize + 0),
                (unsigned long *)(gridDimSize + 1),
                (unsigned long *)(gridDimSize + 2),
                (unsigned long *)(gridDimSize + 3),
                (unsigned long *)(gridDimSize + 4),
                (unsigned long *)(gridDimSize + 5),
                (unsigned long *)(gridDimSize + 6));
        Py_XDECREF(objGridShape);
        if (!ok) {
            bsp_typeError("invalid gridShape for bsp.globalize(procStart,gridShape,up-to-10-array-paths)");
            return Py_BuildValue("O",Py_False);
        }
        for (unsigned iDim = 0; iDim < 7; ++iDim) {
            if (gridDimSize[iDim] > 0)
                ++ nGridDims;
            else
                break;
        }
        std::vector<std::string> arrayPaths;
        for (int i = 0; i < 10; ++i) {
            if (NULL == path[i])
                break;
            arrayPaths.push_back(std::string(path[i]));
        }
        try {
            runtime_->globalize(arrayPaths,nGridDims,gridDimSize,procStart);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.privatize(up-to-10-array-paths)
    static PyObject *bsp_privatize(PyObject *self, PyObject *args) {
        char *path[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
        int ok = PyArg_ParseTuple(args,"s|sssssssss:bsp.privatize",
                path + 0,
                path + 1,
                path + 2,
                path + 3,
                path + 4,
                path + 5,
                path + 6,
                path + 7,
                path + 8,
                path + 9);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.privatize(up-to-10-paths)");
            return Py_BuildValue("O",Py_False);
        }
        std::vector<std::string> arrayPaths;
        for (int i = 0; i < 10; ++i) {
            if (NULL == path[i])
                break;
            arrayPaths.push_back(std::string(path[i]));
        }
        try {
            runtime_->privatize(arrayPaths);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    IndexSet *bsp_regionTensor(unsigned n, char **lowerPath, char **upperPath) {
        LocalArray *lower[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        LocalArray *upper[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        try {
            for (unsigned i = 0; i < n; ++i) {
                NamedObject *nobjLower = runtime_->getObject(std::string(lowerPath[i]));
                lower[i] = nobjLower->_localArray();

                NamedObject *nobjUpper = runtime_->getObject(std::string(upperPath[i]));
                upper[i] = nobjUpper->_localArray();
            }
            return new IndexSetRegionTensor(n,lower,upper);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return NULL;
        }
    }

    IndexSet *bsp_pointTensor(unsigned n, char **lowerPath) {
        LocalArray *lower[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        try {
            for (unsigned i = 0; i < n; ++i) {
                NamedObject *nobjLower = runtime_->getObject(std::string(lowerPath[i]));
                lower[i] = nobjLower->_localArray();
            }
            return new IndexSetPointTensor(n,lower);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return NULL;
        }
    }

    IndexSet *bsp_regionSequence(char *lowerPath, char *upperPath) {
        try {
            NamedObject *nobjLower = runtime_->getObject(std::string(lowerPath));
            LocalArray *lower = nobjLower->_localArray();

            NamedObject *nobjUpper = runtime_->getObject(std::string(upperPath));
            LocalArray *upper = nobjUpper->_localArray();

            return new IndexSetRegionSequence(*lower,*upper);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return NULL;
        }
    }

    IndexSet *bsp_pointSequence(char *lowerPath) {
        try {
            NamedObject *nobjLower = runtime_->getObject(std::string(lowerPath));
            LocalArray *lower = nobjLower->_localArray();

            return new IndexSetPointSequence(*lower);
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return NULL;
        }
    }

    static PyObject *bsp_createPointSet(PyObject *self, PyObject *args) {
        char *lowerPath[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        IndexSet *indexSet = NULL;
        if (PyArg_ParseTuple(args, "s|ssssss:bsp.createPointSet",
                    lowerPath+0,
                    lowerPath+1,
                    lowerPath+2,
                    lowerPath+3,
                    lowerPath+4,
                    lowerPath+5,
                    lowerPath+6
                    )){
            int n = 1;
            for (int i = 1; i < 7; ++i) {
                if (lowerPath[i] == NULL)
                    break;
                ++n;
            }
            if (n > 1) {
                indexSet = bsp_pointTensor(n,lowerPath);
            } else {
                indexSet = bsp_pointSequence(lowerPath[0]);
            }
        } else {
            bsp_typeError("invalid arguments for bsp.createPointSet");
            Py_RETURN_NONE;
        }
        int i = 0;
        while (idToIndexSet_.find(i) != idToIndexSet_.end())
            ++i;
        idToIndexSet_[i] = indexSet;
        indexSetToID_[indexSet] = i;
        return Py_BuildValue("i",i);
    }

    static PyObject *bsp_createRegionSet(PyObject *self, PyObject *args) {
        char *lowerPath[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        char *upperPath[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        IndexSet *indexSet = NULL;
        if (PyArg_ParseTuple(args, "(ss)|(ss)(ss)(ss)(ss)(ss)(ss):bsp.createRegionSet",
                    lowerPath+0,upperPath+0,
                    lowerPath+1,upperPath+1,
                    lowerPath+2,upperPath+2,
                    lowerPath+3,upperPath+3,
                    lowerPath+4,upperPath+4,
                    lowerPath+5,upperPath+5,
                    lowerPath+6,upperPath+6)) {
            int n = 1;
            for (int i = 1; i < 7; ++i) {
                if (lowerPath[i] == NULL)
                    break;
                ++n;
            }
            if (n > 1) {
                indexSet = bsp_regionTensor(7,lowerPath,upperPath);
            } else {
                indexSet = bsp_regionSequence(lowerPath[0],upperPath[0]);
            }
        } else {
            bsp_typeError("invalid arguments for bsp.createRegionSet");
            Py_RETURN_NONE;
        }
        int i = 0;
        while (idToIndexSet_.find(i) != idToIndexSet_.end())
            ++i;
        idToIndexSet_[i] = indexSet;
        indexSetToID_[indexSet] = i;
        return Py_BuildValue("i",i);
    }

    // OK = bsp.deleteIndexSet(indexSet)
    static PyObject *bsp_deleteIndexSet(PyObject *self, PyObject *args) {
        int i = 0;
        int ok = PyArg_ParseTuple(args, "i:bsp.deleteIndexSet",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.deleteIndexSet(indexSet)");
            return Py_BuildValue("O",Py_False);
        }
        try {
            if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                if (activeIndexSet_ == indexSet)
                    activeIndexSet_ = NULL;
                indexSetToID_.erase(indexSet);
                idToIndexSet_.erase(i);
                if (indexSet)
                    delete indexSet;
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // indexCount = bsp.indexCount(indexSet)
    static PyObject *bsp_indexCount(PyObject *self, PyObject *args) {
        int i = 0;
        int ok = PyArg_ParseTuple(args, "i:bsp.indexCount",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.indexCount(indexSet)");
            return Py_BuildValue("i",-1);
        }
        try {
            if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                if (indexSet)
                    return Py_BuildValue("i",indexSet->getNumberOfIndices());
                else
                    return Py_BuildValue("i",0);
            } else 
                return Py_BuildValue("i",0);
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("i",-1);
        }
    }

    // regionCount = bsp.regionCount(indexSet)
    static PyObject *bsp_regionCount(PyObject *self, PyObject *args) {
        int i = 0;
        int ok = PyArg_ParseTuple(args, "i:bsp.regionCount",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.regionCount(indexSet)");
            return Py_BuildValue("i",-1);
        }
        try {
            if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                if (indexSet)
                    return Py_BuildValue("i",indexSet->getNumberOfRegions());
                else
                    return Py_BuildValue("i",0);
            } else 
                return Py_BuildValue("i",0);
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("i",-1);
        }
    }

    // OK = bsp.activateIterator(indexSet)
    static PyObject *bsp_activateIterator(PyObject *self, PyObject *args) {
        int i = 0;
        int ok = PyArg_ParseTuple(args, "i:bsp.activateIterator",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.activateIterator(indexSet)");
            return Py_BuildValue("O",Py_False);
        }
        try {
            if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                activeIndexSet_ = indexSet;
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.resetIterator(optionalIndexSet)
    static PyObject *bsp_resetIterator(PyObject *self, PyObject *args) {
        int i = -1;
        int ok = PyArg_ParseTuple(args, "|i:bsp.resetIterator",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.resetIterator()");
            return Py_BuildValue("O",Py_False);
        }
        try {
            if (i < 0) {
                if (NULL != activeIndexSet_)
                    activeIndexSet_->curr().reset();
                else {
                    bsp_runtimeError("no active index set when calling bsp.resetIterator()");
                    return Py_BuildValue("O",Py_False);
                }
            } else if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                indexSet->curr().reset();
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    PyObject *bsp_buildIndex(IndexSet *indexSet) {
        int nDims = indexSet->getNumberOfDimensions();
        uint64_t index[7];
        IndexSet::Iterator &iter = indexSet->curr();
        for (int iDim = 0; iDim < nDims; ++iDim) {
            index[iDim] = iter.getIndex(iDim);
        }
        switch (nDims) {
            case 1:
                return Py_BuildValue("k",(unsigned long)index[0]);
            case 2:
                return Py_BuildValue("(kk)",
                        (unsigned long)index[0],
                        (unsigned long)index[1]
                        );
            case 3:
                return Py_BuildValue("(kkk)",
                        (unsigned long)index[0],
                        (unsigned long)index[1],
                        (unsigned long)index[2]
                        );
            case 4:
                return Py_BuildValue("(kkkk)",
                        (unsigned long)index[0],
                        (unsigned long)index[1],
                        (unsigned long)index[2],
                        (unsigned long)index[3]
                        );
            case 5:
                return Py_BuildValue("(kkkkk)",
                        (unsigned long)index[0],
                        (unsigned long)index[1],
                        (unsigned long)index[2],
                        (unsigned long)index[3],
                        (unsigned long)index[4]
                        );
            case 6:
                return Py_BuildValue("(kkkkkk)",
                        (unsigned long)index[0],
                        (unsigned long)index[1],
                        (unsigned long)index[2],
                        (unsigned long)index[3],
                        (unsigned long)index[4],
                        (unsigned long)index[5]
                        );
            default:
                return Py_BuildValue("(kkkkkkk)",
                        (unsigned long)index[0],
                        (unsigned long)index[1],
                        (unsigned long)index[2],
                        (unsigned long)index[3],
                        (unsigned long)index[4],
                        (unsigned long)index[5],
                        (unsigned long)index[6]
                        );
        }
    }

    // currIndex = bsp.currentIndex(optionalIndexSet) 
    static PyObject *bsp_currentIndex(PyObject *self, PyObject *args) {
        int i = -1;
        int ok = PyArg_ParseTuple(args, "|i:bsp.currentIndex",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.currentIndex()");
            Py_RETURN_NONE;
        }
        try {
            if (i < 0) {
                if (NULL != activeIndexSet_) {
                    return bsp_buildIndex(activeIndexSet_);
                }
                else {
                    bsp_runtimeError("no active index set when calling bsp.currentIndex()");
                    Py_RETURN_NONE;
                }
            } else if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                return bsp_buildIndex(indexSet);
            } else {
                Py_RETURN_NONE;
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            Py_RETURN_NONE;
        }
    }

    // nextIndex = bsp.nextIndex(optionalIndexSet)
    static PyObject *bsp_nextIndex(PyObject *self, PyObject *args) {
        int i = -1;
        int ok = PyArg_ParseTuple(args, "|i:bsp.nextIndex",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.nextIndex()");
            Py_RETURN_NONE;
        }
        try {
            if (i < 0) {
                if (NULL != activeIndexSet_) {
                    ++ activeIndexSet_->curr();
                    if (activeIndexSet_->curr() == activeIndexSet_->end())
                        Py_RETURN_NONE;
                    return bsp_buildIndex(activeIndexSet_);
                }
                else {
                    bsp_runtimeError("no active index set when calling bsp.nextIndex()");
                    Py_RETURN_NONE;
                }
            } else if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                ++ indexSet->curr();
                if (indexSet->curr() == indexSet->end())
                    Py_RETURN_NONE;
                return bsp_buildIndex(indexSet);
            } else {
                Py_RETURN_NONE;
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            Py_RETURN_NONE;
        }
    }

    // indexOfNextRegion = bsp.nextRegion(optionalIndexSet)
    static PyObject *bsp_nextRegion(PyObject *self, PyObject *args) {
        int i = -1;
        int ok = PyArg_ParseTuple(args, "|i:bsp.nextRegion",&i);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.nextRegion()");
            Py_RETURN_NONE;
        }
        Py_RETURN_NONE;
        try {
            if (i < 0) {
                if (NULL != activeIndexSet_) {
                    activeIndexSet_->curr().nextRegion();
                    if (activeIndexSet_->curr() == activeIndexSet_->end())
                        Py_RETURN_NONE;
                    return bsp_buildIndex(activeIndexSet_);
                }
                else {
                    bsp_runtimeError("no active index set when calling bsp.nextRegion()");
                    Py_RETURN_NONE;
                }
            } else if (idToIndexSet_.find(i) != idToIndexSet_.end()) {
                IndexSet *indexSet = idToIndexSet_[i];
                indexSet->curr().nextRegion();
                if (indexSet->curr() == indexSet->end())
                    Py_RETURN_NONE;
                return bsp_buildIndex(indexSet);
            } else {
                Py_RETURN_NONE;
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            Py_RETURN_NONE;
        }
    }

    // OK = bsp.requestTo(clientArrayPath,serverArrayPath,indexSet,optionalServerProcID)
    static PyObject *bsp_requestTo(PyObject *self, PyObject *args) {
        char *clientPath = NULL;
        char *serverPath = NULL;
        int indexSetID = -1;
        long serverProcID = -1;
        int ok = PyArg_ParseTuple(args, "ssi|l:bsp.requestTo",&clientPath,&serverPath,&indexSetID,&serverProcID);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.requestTo(clientArrayPath,"
                    "serverArrayPath,indexSet,optionalServerProcID)");
            return Py_BuildValue("O",Py_False);
        }
        try {
            LocalArray *clientArray = runtime_->getObject(clientPath)->_localArray();
            NamedObject *nobjServer = runtime_->getObject(serverPath);
            if (ARRAY != nobjServer->getType()) {
                bsp_typeError("invalid serverArrayPath for bsp.requestTo("
                        "clientArrayPath,serverArrayPath,indexSet,optionalServerProcID)");
                return Py_BuildValue("O",Py_False);
            }
            if (idToIndexSet_.find(indexSetID) == idToIndexSet_.end()) {
                PyErr_SetString(PyExc_TypeError, "invalid indexSet for bsp.requestTo("
                        "clientArrayPath,serverArrayPath,indexSet,optionalServerProcID)");
                return Py_BuildValue("O",Py_False);
            }
            if (nobjServer->isGlobal()) {
                if (serverProcID >= 0) {
                    bsp_typeError( 
                            "serverProcID not required for local shared client "
                            "in bsp.requestTo(clientArrayPath,serverArrayPath,indexSet,optionalServerProcID)");
                    return Py_BuildValue("O",Py_False);
                } else {
                    runtime_->requestFrom(*nobjServer->_globalArray(),
                            *idToIndexSet_[indexSetID],
                            *clientArray, bsp_getScriptPos());
                }
            } else {
                if (serverProcID < 0) {
                    bsp_typeError("serverProcID required for global client"
                            " in bsp.requestTo(clientArrayPath,serverArrayPath,indexSet,optionalServerProcID)");
                    return Py_BuildValue("O",Py_False);
                } else {
                    runtime_->requestFrom(*nobjServer->_localArray(), (uint64_t)serverProcID,
                            *idToIndexSet_[indexSetID],
                            *clientArray, bsp_getScriptPos());
                }
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.updateFrom(clientArrayPath,op,serverArrayPath,indexSet,optionalServerProcID)
    static PyObject *bsp_updateFrom(PyObject *self, PyObject *args) {
        char *clientPath = NULL;
        char *serverPath = NULL;
        char *op = NULL;
        int indexSetID = -1;
        long serverProcID = -1;
        uint16_t opID = LocalArray::OPID_ASSIGN;
        int ok = PyArg_ParseTuple(args, "sssi|l:bsp.requestTo",&clientPath,&op,&serverPath,&indexSetID,&serverProcID);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.requestFrom(clientArrayPath,op,"
                    "serverArrayPath,indexSet,optionalServerProcID)");
            return Py_BuildValue("O",Py_False);
        }
        try {
            LocalArray *clientArray = runtime_->getObject(clientPath)->_localArray();
            NamedObject *nobjServer = runtime_->getObject(serverPath);
            if (ARRAY != nobjServer->getType()) {
                bsp_typeError("invalid serverArrayPath for bsp.requestFrom("
                        "clientArrayPath,op,serverArrayPath,indexSet,optionalServerProcID)");
                return Py_BuildValue("O",Py_False);
            }
            if (strcmp(op, "="))
                opID = LocalArray::OPID_ASSIGN;
            else if (strcmp(op, "+") || strcmp(op, "+="))
                opID = LocalArray::OPID_ADD;
            else if (strcmp(op, "*") || strcmp(op, "*="))
                opID = LocalArray::OPID_MUL;
            else if (strcmp(op, "&") || strcmp(op, "&="))
                opID = LocalArray::OPID_AND;
            else if (strcmp(op, "|") || strcmp(op, "|="))
                opID = LocalArray::OPID_OR;
            else if (strcmp(op, "^") || strcmp(op, "^="))
                opID = LocalArray::OPID_XOR;
            else if (strcmp(op, "min") || strcmp(op, "min="))
                opID = LocalArray::OPID_MIN;
            else if (strcmp(op, "max") || strcmp(op, "max="))
                opID = LocalArray::OPID_MAX;
            else {
                bsp_typeError("unrecognized op for bsp.requestFrom("
                        "clientArrayPath,op,serverArrayPath,indexSet,optionalServerProcID)");
                return Py_BuildValue("O",Py_False);
            }
            if (idToIndexSet_.find(indexSetID) == idToIndexSet_.end()) {
                PyErr_SetString(PyExc_TypeError, "invalid indexSet for bsp.requestFrom("
                        "clientArrayPath,op,serverArrayPath,indexSet,optionalServerProcID)");
                return Py_BuildValue("O",Py_False);
            }
            if (nobjServer->isGlobal()) {
                if (serverProcID >= 0) {
                    bsp_typeError( 
                            "serverProcID not required for local shared client "
                            "in bsp.requestFrom(clientArrayPath,op,serverArrayPath,indexSet,optionalServerProcID)");
                    return Py_BuildValue("O",Py_False);
                } else {
                    runtime_->requestTo(*nobjServer->_globalArray(),
                            *idToIndexSet_[indexSetID],
                            *clientArray, opID, bsp_getScriptPos());
                }
            } else {
                if (serverProcID < 0) {
                    bsp_typeError("serverProcID required for global client"
                            " in bsp.requestFrom(clientArrayPath,op,serverArrayPath,indexSet,optionalServerProcID)");
                    return Py_BuildValue("O",Py_False);
                } else {
                    runtime_->requestTo(*nobjServer->_localArray(), (uint64_t)serverProcID,
                            *idToIndexSet_[indexSetID],
                            *clientArray, opID, bsp_getScriptPos());
                }
            }
        } catch (const std::exception& e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    // OK = bsp.toProc(procID,up-to-10-local-arrays)
    static PyObject *bsp_toProc(PyObject *self, PyObject *args) {
        unsigned long procID = 0;
        char *path[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
        int ok = PyArg_ParseTuple(args, "ks|sssssssss:bsp.toProc",
                &procID,
                path + 0,
                path + 1,
                path + 2,
                path + 3,
                path + 4,
                path + 5,
                path + 6,
                path + 7,
                path + 8,
                path + 9);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.toProc(procID,up-to-10-local-arrays)");
            return Py_BuildValue("O",Py_False);
        }
        try {
            for (int i = 0; i < 10; ++i) {
                if (NULL == path[i])
                    break;
                runtime_->exportUserDefined(procID,std::string(path[i]));
            }
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    void bsp_extractImported(NamedObject *nobj, std::string prefix, PyObject *pyDict) {
        if (nobj->getType() == ARRAY && !nobj->isGlobal()) {
            LocalArray *localArray = nobj->_localArray();
            std::string strKey;
            if (prefix == "")
                strKey = nobj->getName();
            else
                strKey = prefix + "." + nobj->getName();
            PyObject *value = Py_BuildValue("s",localArray->getPath().c_str());
            PyDict_SetItemString(pyDict,strKey.c_str(),value);
        } else if (nobj->getType() == NAMESPACE) {
            NameSpace *nsp = nobj->_namespace();
            std::string myPrefix; 
            if (prefix == "")
                myPrefix = nobj->getName();
            else
                myPrefix = prefix + "." + nobj->getName();
            for (NameSpaceIterator iter = nsp->begin(); iter != nsp->end(); ++iter) {
                bsp_extractImported(iter->second, myPrefix, pyDict);
            }
        }
    }

    // OK = bsp.sync(tag,optionalSendMatrix)
    static PyObject *bsp_sync(PyObject *self, PyObject *args) {
        char *tag = NULL;
        PyObject *objSendMatrix = NULL;
        int ok = PyArg_ParseTuple(args, "s|O:bsp.sync", &tag, &objSendMatrix);
        Py_XINCREF(objSendMatrix);
        if (!ok) {
            bsp_typeError("invalid arguments for bsp.sync(tag,optionalSendMatrix)");
            Py_XDECREF(objSendMatrix);
            return Py_BuildValue("O",Py_False);
        }
        try {
            if (NULL != objSendMatrix) {
                if (!PyArray_Check(objSendMatrix)) {
                    bsp_typeError("invalid sendMatrix for bsp.sync(tag,optionalSendMatrix)");
                    Py_XDECREF(objSendMatrix);
                    return Py_BuildValue("O",Py_False);
                } else {
                    PyArrayObject *arrSendMatrix = (PyArrayObject *)objSendMatrix;
                    int nDims = PyArray_NDIM(arrSendMatrix);
                    if (2 != nDims || !PyArray_ISBOOL(arrSendMatrix)) {
                        bsp_typeError("invalid sendMatrix for bsp.sync(tag,optionalSendMatrix)");
                        Py_XDECREF(objSendMatrix);
                        return Py_BuildValue("O",Py_False);
                    } else {
                        if (PyArray_DIM(arrSendMatrix,0) != nProcs_ ||
                                PyArray_DIM(arrSendMatrix,1) != nProcs_) {
                            bsp_typeError("invalid sendMatrix for bsp.sync(tag,optionalSendMatrix)");
                            Py_XDECREF(objSendMatrix);
                            return Py_BuildValue("O",Py_False);
                        } else {
                            bool *sendMatrix = new bool[nProcs_ * nProcs_];
                            bool *data = (bool *)PyArray_DATA(arrSendMatrix);
                            uint64_t k = 0;
                            for (uint64_t i = 0; i < nProcs_; ++i) {
                                for (uint64_t j = 0; j < nProcs_; ++j) {
                                    sendMatrix[k] = data[k];
                                    ++k;
                                }
                            }
                            Py_XDECREF(objSendMatrix);
                            runtime_->exchange(sendMatrix, tag);
                            delete[] sendMatrix;
                        }
                    }
                }
            } else {
                bool *sendMatrix = new bool[nProcs_ * nProcs_];
                uint64_t k = 0;
                for (uint64_t i = 0; i < nProcs_; ++ i) {
                    for (uint64_t j = 0; j < nProcs_; ++ j) {
                        sendMatrix[k ++] = true;
                    }
                }
                runtime_->exchange(sendMatrix, tag);
                delete[] sendMatrix;
            }

            // update the fromProc lists
            for (uint64_t iProc = 0; iProc < nProcs_; ++iProc) {
                // clear the fromProc list
                PyObject *key, *value;
                Py_ssize_t pos = 0;
                while (PyDict_Next(fromProc_[iProc],&pos,&key,&value)){
                    Py_XDECREF(value);
                }
                PyDict_Clear(fromProc_[iProc]);

                // found all local arrays in the import path
                std::stringstream ss;
                ss << "_import.procID" << iProc;
                std::string pathImport = ss.str();
                if (runtime_->hasObject(pathImport)) {
                    NamedObject *nobjPath = runtime_->getObject(pathImport);
                    NameSpace *nspPath = nobjPath->_namespace();
                    for (NameSpaceIterator iter = nspPath->begin(); iter != nspPath->end(); ++iter)
                        bsp_extractImported(iter->second,"",fromProc_[iProc]);
                }
            }
            
        } catch (const std::exception &e) {
            bsp_runtimeError(e.what());
            return Py_BuildValue("O",Py_False);
        }
        return Py_BuildValue("O",Py_True);
    }

    static PyMethodDef bspMethods_[] = {
        {"myProcID",bsp_myProcID,METH_VARARGS,"get the rank of current process"},
        {"procCount",bsp_procCount,METH_VARARGS,"get the number of processes"},
        {"fromProc",bsp_fromProc,METH_VARARGS,"get the imported objects from a given process"},
        {"toProc",bsp_toProc,METH_VARARGS,"export an object to a given process"},
        {"fromObject",bsp_fromObject,METH_VARARGS,"build local array from an object"},
        {"fromNumpy",bsp_fromNumpy,METH_VARARGS,"build local array from a numpy array"},
        {"toObject",bsp_toObject,METH_VARARGS,"build an object from a local array"},
        {"toNumpy",bsp_toNumpy,METH_VARARGS,"build a numpy array from a local aray"},
        {"asNumpy",bsp_asNumpy,METH_VARARGS,"open a view of numpy array for accessing a local array"},
        {"createArray",bsp_createArray,METH_VARARGS,"create a new local array"},
        {"delete",bsp_delete,METH_VARARGS,"delete a local array or a path"},
        {"share",bsp_share,METH_VARARGS,"share local arrays"},
        {"globalize",bsp_globalize,METH_VARARGS,"globalize local arrays"},
        {"privatize",bsp_privatize,METH_VARARGS,"privatize share/global arrays"},
        {"createRegionSet",bsp_createRegionSet,METH_VARARGS,"create an region index-set"},
        {"createPointSet",bsp_createPointSet,METH_VARARGS,"create an Point index-set"},
        {"deleteIndexSet",bsp_deleteIndexSet,METH_VARARGS,"delete an index-set"},
        {"indexCount",bsp_indexCount,METH_VARARGS,"get the count of indices in an index set"},
        {"regionCount",bsp_regionCount,METH_VARARGS,"get the count of regions in an index set"},
        {"resetIterator",bsp_resetIterator,METH_VARARGS,"reset the iterator of an index set"},
        {"activateIterator",bsp_activateIterator,METH_VARARGS,"activate the iterator of an index set"},
        {"currentIndex",bsp_currentIndex,METH_VARARGS,"get current index of an index set"},
        {"nextIndex",bsp_nextIndex,METH_VARARGS,"get next index of an index set"},
        {"nextRegion",bsp_nextRegion,METH_VARARGS,"get the first index of next region in an index set"},
        {"requestTo",bsp_requestTo,METH_VARARGS,"request data from a share/global array to a local array"},
        {"updateFrom",bsp_updateFrom,METH_VARARGS,"update data from a local array to a share/global array"},
        {"sync",bsp_sync,METH_VARARGS,"sync data with optional send-matrix"},
        {NULL,NULL,0,NULL}
    };

    void initBSP(int *pArgc, char ***pArgv) {
        runtime_ = new Runtime(pArgc, pArgv);
        //runtime_->setVerbose(true);
        nProcs_ = runtime_->getNumberOfProcesses();
        fromProc_ = new PyObject *[nProcs_];
        for (unsigned i = 0; i < nProcs_; ++i) {
            fromProc_[i] = Py_BuildValue("{}");
        }

        Py_SetProgramName((*pArgv)[0]);
        Py_Initialize();
        import_array();

        cPickle_ = PyImport_ImportModule("cPickle");
        Py_XINCREF(cPickle_);

        traceback_ = PyImport_ImportModule("traceback");
        Py_XINCREF(traceback_);

        cPickle_dumps_ = PyObject_GetAttrString(cPickle_, "dumps");
        assert(PyCallable_Check(cPickle_dumps_));
        Py_XINCREF(cPickle_dumps_);

        cPickle_loads_ = PyObject_GetAttrString(cPickle_, "loads");
        assert(PyCallable_Check(cPickle_loads_));
        Py_XINCREF(cPickle_loads_);

        traceback_extractStack_ = PyObject_GetAttrString(traceback_, "extract_stack");
        Py_XINCREF(traceback_extractStack_);

        PyObject *mbsp = Py_InitModule("bsp",bspMethods_);
        if (NULL == mbsp) {
            PyErr_SetString(PyExc_RuntimeError, "failed to init bsp module");
            finiBSP();
            exit(-1);
        }
    }

}
