/*
 * BSPScope.hpp
 *
 *  Created on: 2014-7-10
 *      Author: junfeng
 */

#ifndef BSPSCOPE_HPP_
#define BSPSCOPE_HPP_
#include <string>
#include <map>
#include "BSPNamedObject.hpp"
namespace BSP {
    typedef std::map<std::string, NamedObject *>::iterator NameSpaceIterator;

    class NameSpace {
    private:
        std::map<std::string, NamedObject *> _objects;
        std::string _path;
    public:
        NameSpace(std::string path = "this");
        virtual ~NameSpace();
        NamedObject *getObject(std::string objectName);
        void setObject(std::string objectName, NamedObject *object);
        void deleteObject(std::string objectName, bool collective);
        void clear();
        std::string list();
        NameSpaceIterator begin(){return _objects.begin();}
        NameSpaceIterator end(){return _objects.end();}
        const std::string getPath() const {return _path;}
    };


}

#endif /* BSPSCOPE_HPP_ */
