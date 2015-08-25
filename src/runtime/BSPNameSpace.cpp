/*
 * BSPScope.cpp
 *
 *  Created on: 2014-7-12
 *      Author: junfeng
 */

#include "BSPNameSpace.hpp"
#include "BSPLocalArray.hpp"
#include "BSPRuntime.hpp"
#include <sstream>

using namespace BSP;

NameSpace::NameSpace(std::string path) {
    _path = path;
}

NameSpace::~NameSpace() {
    clear();
}

NamedObject *NameSpace::getObject(std::string objectName) {
    if (_objects.find(objectName) == _objects.end())
        return NULL;
    else
        return _objects[objectName];
}

void NameSpace::setObject(std::string objectName, NamedObject *object) {
    if (_objects.find(objectName) == _objects.end())
        _objects[objectName] = object;
    else {
        delete _objects[objectName];
        _objects[objectName] = object;
    }
}

void NameSpace::deleteObject(std::string objectName, bool collective) {
    std::map<std::string, NamedObject*>::iterator iterator = _objects.find(
            objectName);
    if (iterator == _objects.end())
        return;
    if (!collective) {
        if (iterator->second->getType() == ARRAY) {
            if (iterator->second->isGlobal()) {
                throw EDeleteSharedArray((_path + "." + objectName).c_str());
            } else if (iterator->second->_localArray()->getRegistration()) {
                throw EDeleteSharedArray((_path + "." + objectName).c_str());
            }
        }
    }
    delete iterator->second;
    _objects.erase(iterator);
}

void NameSpace::clear() {
    bool finalizing = BSP::Runtime::getActiveRuntime()->isFinalizing();
    for (std::map<std::string, NamedObject*>::iterator iterator =
            _objects.begin(); iterator != _objects.end(); ++iterator) {
        if (!finalizing && iterator->second->getType() == ARRAY) {
            if (iterator->second->isGlobal()) {
                throw EDeleteSharedArray((_path + "." + iterator->first).c_str());
            } else if (iterator->second->_localArray()->getRegistration()) {
                throw EDeleteSharedArray((_path + "." + iterator->first).c_str());
            }
        }
        delete iterator->second;
    }
    _objects.clear();
}

std::string NameSpace::list() {
    std::stringstream ss;
    ss << "{";
    for (NameSpaceIterator iterator =
            _objects.begin(); iterator != _objects.end(); ++iterator) {
        ss << iterator->second->getName();
        switch (iterator->second->getType()) {
            case ARRAY:
            {
                ss << ":ARR";
                break;
            }
            case NAMESPACE:
            {
                ss << ":NSP";
                break;
            }
            default:
            {
                throw ECorruptedRuntime();
                break;
            }
        }
        ss << ";";
    }
    ss << "}";
    return ss.str();
}


