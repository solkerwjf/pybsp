#ifndef BSPNAMEDOBJECT_HPP_
#define BSPNAMEDOBJECT_HPP_
#include <string>
#include <stdint.h>
#include "BSPException.hpp"
namespace BSP {
class GlobalArray;
class LocalArray;
class NameSpace;
enum Type {
	ARRAY, NAMESPACE
};

class Value {
protected:
	std::string _name;
private:
	Type _type;
	bool _isGlobal;
	void release();
	union {
		union {
			GlobalArray *_array;
		} _global;
		union {
			LocalArray *_array;
			NameSpace *_namespace;
		} _local;
	};
public:
	Value(GlobalArray *globalArray);
	Value(LocalArray *localArray);
	Value(NameSpace *scope);
	virtual ~Value();
	const Type getType() const {
		return _type;
	}
	const bool isGlobal() const {
		return _isGlobal;
	}
	Value &operator =(const Value &value);

	GlobalArray *_globalArray() {
	        //if (this == NULL)
		//    throw EInvalidArgument();
		if (_type != ARRAY || !_isGlobal)
			throw ETypeNotMatched(_name.c_str(), "global array");
		return _global._array;
	}
	LocalArray *_localArray() {
	        //if (this == NULL)
		//    throw EInvalidArgument();
		if (_type != ARRAY || _isGlobal)
			throw ETypeNotMatched(_name.c_str(), "local array");
		return _local._array;
	}
	NameSpace *_namespace() {
	        //if (this == NULL)
		//    throw EInvalidArgument();
		if (_type != NAMESPACE)
			throw ETypeNotMatched(_name.c_str(), "name space");
		return _local._namespace;
	}
};

class NamedObject: public Value {
public:
	NamedObject(std::string name, GlobalArray *globalArray) :
			Value(globalArray) {
	    _name=name;
	}

	NamedObject(std::string name, LocalArray *localArray) :
			Value(localArray) {
	    _name=name;
	}

	NamedObject(std::string name, NameSpace *scope) :
			Value(scope) {
	    _name=name;
	}

	std::string getName() const {
		return _name;
	}

};

}

#endif /* BSPNAMEDOBJECT_HPP_ */
