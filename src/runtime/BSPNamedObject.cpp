/*
 * BSPNamedObject.cpp
 *
 *  Created on: 2014-7-10
 *      Author: junfeng
 */

#include "BSPNamedObject.hpp"
#include "BSPRuntime.hpp"
#include "BSPGlobalArray.hpp"
#include "BSPLocalArray.hpp"
#include "BSPIndexSet.hpp"
#include "BSPGrid.hpp"
#include "BSPNameSpace.hpp"
namespace BSP {

    Value::Value(GlobalArray *globalArray) {
        _type = ARRAY;
        _isGlobal = true;
        _global._array = globalArray;
    }

    Value::Value(LocalArray *localArray) {
        _type = ARRAY;
        _isGlobal = false;
        _local._array = localArray;
    }

    Value::Value(NameSpace *scope) {
        _type = NAMESPACE;
        _isGlobal = false;
        _local._namespace = scope;
    }

    void Value::release() {
        if (_isGlobal) {
            switch (_type) {
                case ARRAY:
                {
                    delete _global._array;
                    break;
                }
                default:
                    break;
            }
        } else {
            switch (_type) {
                case ARRAY:
                {
                    delete _local._array;
                    break;
                }
                case NAMESPACE:
                {
                    delete _local._namespace;
                    break;
                }
                default:
                    break;
            }
        }
    }

    Value::~Value() {
        release();
    }

}
