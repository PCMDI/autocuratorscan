///////////////////////////////////////////////////////////////////////////////
///
///	\file    Object.h
///	\author  Paul Ullrich
///	\version March 8, 2017
///
///	<remarks>
///		Copyright 2016- Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "Exception.h"
#include "GlobalFunction.h"
#include "ObjectType.h"

#include <string>
#include <set>
#include <vector>
#include <map>

///////////////////////////////////////////////////////////////////////////////

class VariableRegistry;

class Object;

typedef std::vector<Object *> ObjectChildrenVector;

typedef std::map<std::string, Object *> ObjectMap;

typedef int ObjectIndex;

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for registering Objects.
///	</summary>
class ObjectRegistry {

public:
	///	<summary>
	///		Destructor.
	///	</summary>
	~ObjectRegistry();

public:
	///	<summary>
	///		Get the Object with the specified name.
	///	</summary>
	Object * GetObject(
		const std::string & strName
	) const;

	///	<summary>
	///		Create an Object of the given type.
	///	</summary>
	bool Create(
		const ObjectType & objtype,
		const std::string & strName,
		const std::string & strValue
	);

	///	<summary>
	///		Remove the Object with the specified name.
	///	</summary>
	std::string Remove(
		const std::string & strName
	);

	///	<summary>
	///		Assign the Object with the specified name.
	///	</summary>
	///	<returns>
	///		true if insertion is successful.  false if parent Object could
	///		not be found in the ObjectRegistry.
	///	</returns>
	bool Assign(
		const std::string & strName,
		Object * pObject
	);

private:
	///	<summary>
	///		Map from Object name to Object instance.
	///	</summary>
	ObjectMap m_mapObjects;
};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A GlobalFunction that builds a new Object.
///	</summary>
class ObjectConstructor : public GlobalFunction {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	ObjectConstructor(const std::string & strName) :
		GlobalFunction(strName)
	{ }

public:
	///	<summary>
	///		Call a member function of this GlobalFunction.
	///	</summary>
	virtual std::string Call(
		const ObjectRegistry & objreg,
		const std::vector<std::string> & vecCommandLine,
		const std::vector<ObjectType> & vecCommandLineType,
		Object ** ppReturn
	);
};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for representing objects.
///	</summary>
class Object {

friend class ObjectRegistry;

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	Object(const std::string & strName) :
		m_strName(strName),
		m_nLocks(0)
	{ }

	///	<summary>
	///		Virtual destructor.
	///	</summary>
	virtual ~Object() {
	}

	///	<summary>
	///		Get the name of this Object.
	///	</summary>
	const std::string & Name() const {
		return m_strName;
	}

	///	<summary>
	///		Create a string containing this Object's name.
	///	</summary>
	std::string ChildName(
		const std::string & strChild
	) const {
		return (m_strName + "." + strChild);
	}

	///	<summary>
	///		Notify object of its deletion.
	///	</summary>
	virtual void PrepareDelete() {
	}

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(new Object(strDuplicateName), objreg);
	}

	///	<summary>
	///		Call a member function of this Object.
	///	</summary>
	virtual std::string Call(
		const ObjectRegistry & objreg,
		const std::string & strFunctionName,
		const std::vector<std::string> & vecCommandLine,
		const std::vector<ObjectType> & vecCommandLineType,
		Object ** ppReturn
	) {
		return std::string("ERROR: Unknown member function [")
			+ strFunctionName + std::string("]");
	}

	///	<summary>
	///		Number of children in Object.
	///	</summary>
	size_t ChildrenCount() const {
		return m_vecChildren.size();
	}

	///	<summary>
	///		Get child Object by index.
	///	</summary>
	Object * GetChild(size_t sChild) const {
		if (sChild >= m_vecChildren.size()) {
			_EXCEPTIONT("Children vector access out of range");
		}
		return m_vecChildren[sChild];
	}

	///	<summary>
	///		Get child Object by name.
	///	</summary>
	Object * GetChild(const std::string & strChildName) const {
		std::string strFullChildName = ChildName(strChildName);
		for (size_t i = 0; i < m_vecChildren.size(); i++) {
			if (m_vecChildren[i]->Name() == strFullChildName) {
				return m_vecChildren[i];
			}
		}
		return NULL;
	}

	///	<summary>
	///		Add a child Object.
	///	</summary>
	virtual bool AddUnassignedChild(Object * pChild) {
		if (m_strName != "__TEMPNAME__") {
			_EXCEPTIONT("Not a temporary object");
		}
		m_vecChildrenUnassigned.push_back(pChild);
		return true;
	}

protected:
	///	<summary>
	///		Add a child Object.
	///	</summary>
	virtual bool AddChild(Object * pChild) {
		m_vecChildren.push_back(pChild);
		return true;
	}

protected:
	///	<summary>
	///		Duplicate this object and its children.
	///	</summary>
	Object * _Duplicate(
		Object * pobjDuplicate,
		ObjectRegistry & objreg
	) const;

public:
	///	<summary>
	///		Add a lock to this Object.
	///	</summary>
	virtual void AddLock() {
		m_nLocks++;
	}

	///	<summary>
	///		Release a lock from this Object.
	///	</summary>
	virtual void ReleaseLock() {
		if (m_nLocks == 0) {
			_EXCEPTIONT("No locks on object");
		}
		m_nLocks--;
	}

	///	<summary>
	///		Check if this Object is locked.
	///	</summary>
	bool IsLocked() const {
		return (m_nLocks > 0);
	}

protected:
	///	<summary>
	///		Name of the Object.
	///	</summary>
	std::string m_strName;

	///	<summary>
	///		List of unassigned child Objects.
	///	</summary>
	ObjectChildrenVector m_vecChildrenUnassigned;

	///	<summary>
	///		List of child Objects.
	///	</summary>
	ObjectChildrenVector m_vecChildren;

private:
	///	<summary>
	///		Number of locks placed on this Object.  An object with a 
	///		non-zero lock count cannot be deleted.
	///	</summary>
	int m_nLocks;
};

///	<summary>
///		A class for representing Objects that may be distributed
///		across MPI ranks.
///	</summary>
class DistributedObject : public Object {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	DistributedObject(const std::string & strName) :
		Object(strName),
		m_fDistributed(true)
	{ }

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(
			new DistributedObject(strDuplicateName),
			objreg);
	}

	///	<summary>
	///		Set the distributed flag to true.
	///	</summary>
	void SetDistributed() {
		m_fDistributed = true;
	}

	///	<summary>
	///		Set the distributed flag to false.
	///	</summary>
	void UnsetDistributed() {
		m_fDistributed = false;
	}

	///	<summary>
	///		Get the state of the distributed flag.
	///	</summary>
	bool IsDistributed() const {
		return m_fDistributed;
	}

protected:
	///	<summary>
	///		A flag indicating data remains distributed across ranks.
	///	</summary>
	bool m_fDistributed;
};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for representing String objects.
///	</summary>
class StringObject : public Object {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	StringObject(
		const std::string & strName,
		const std::string & strValue
	) :
		Object(strName),
		m_strValue(strValue)
	{ }

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(
			new StringObject(strDuplicateName, m_strValue),
			objreg);
	}

	///	<summary>
	///		Get the string.
	///	</summary>
	const std::string & Value() const {
		return m_strValue;
	}

public:
	///	<summary>
	///		Return the value of the string with the specified units.
	///	</summary>
	bool ToUnit(
		const std::string & strUnit,
		double * dValueOut,
		bool fIsDelta = false
	);

protected:
	///	<summary>
	///		String value of the Object.
	///	</summary>
	std::string m_strValue;

};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for representing integer objects.
///	</summary>
class IntegerObject : public Object {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	IntegerObject(
		const std::string & strName,
		const int & iValue
	) :
		Object(strName),
		m_iValue(iValue)
	{ }

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(
			new IntegerObject(strDuplicateName, m_iValue),
			objreg);
	}

	///	<summary>
	///		Get the integer value.
	///	</summary>
	const int & Value() const {
		return m_iValue;
	}

protected:
	///	<summary>
	///		Integer value of the Object.
	///	</summary>
	int m_iValue;

};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for representing floating point objects.
///	</summary>
class FloatingPointObject : public Object {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	FloatingPointObject(
		const std::string & strName,
		const double & dValue
	) :
		Object(strName),
		m_dValue(dValue)
	{ }

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(
			new FloatingPointObject(strDuplicateName, m_dValue),
			objreg);
	}

protected:
	///	<summary>
	///		Double value of the Object.
	///	</summary>
	double m_dValue;

};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for representing Variable operations.
///	</summary>
class VariableObject : public Object {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	VariableObject(
		const std::string & strName,
		const std::string & strValue
	) :
		Object(strName),
		m_strValue(strValue)
	{ }

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(
			new VariableObject(strDuplicateName, m_strValue),
			objreg);
	}

protected:
	///	<summary>
	///		String describing the Variable operation.
	///	</summary>
	std::string m_strValue;
};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for registering Objects.
///	</summary>
class ListObject : public Object {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	ListObject(
		const std::string & strName
	) :
		Object(strName)
	{ }

	///	<summary>
	///		Self-duplicator.
	///	</summary>
	virtual Object * Duplicate(
		const std::string & strDuplicateName,
		ObjectRegistry & objreg
	) const {
		return _Duplicate(
			new ListObject(strDuplicateName),
			objreg);
	}

public:
	///	<summary>
	///		Add an Object to the ListObject.
	///	</summary>
	void PushBack(
		const std::string & strObject
	) {
		if (m_vecObjectNames.size() > 10000) {
			_EXCEPTIONT("Maximum of 10000 Objects allowed in ListObject");
		}
		m_vecObjectNames.push_back(strObject);
	}

	///	<summary>
	///		Number of items in the ListObject.
	///	</summary>
	size_t Count() const {
		return m_vecObjectNames.size();
	}

	///	<summary>
	///		Get the name of the nth item in the ListObject.
	///	</summary>
	std::string ChildName(size_t n) const {
		return Object::ChildName(std::string("_") + std::to_string(n));
	}

protected:
	///	<summary>
	///		Vector of Object names in this ListObject.
	///	</summary>
	std::vector<std::string> m_vecObjectNames;

};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A GlobalFunction that builds a new ListObject.
///	</summary>
class ListObjectSpanConstructor : public GlobalFunction {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	ListObjectSpanConstructor(const std::string & strName) :
		GlobalFunction(strName)
	{ }

public:
	///	<summary>
	///		Call a member function of this GlobalFunction.
	///	</summary>
	virtual std::string Call(
		const ObjectRegistry & objreg,
		const std::vector<std::string> & vecCommandLine,
		const std::vector<ObjectType> & vecCommandLineType,
		Object ** ppReturn
	);
};

///////////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Try to convert a value/unit pair to the target units.
///	</summary>
bool ConvertUnits(
	double dValueIn,
	const std::string & strUnit,
	double & dValueOut,
	const std::string & strTargetUnit,
	bool fIsDelta
);

///	<summary>
///		Try to convert the string to have the specified units.
///	</summary>
bool StringToValueUnit(
	const std::string & strString,
	const std::string & strTargetUnit,
	double & dValue,
	bool fIsDelta
);

///	<summary>
///		Determine if two units are compatible.
///	</summary>
bool AreUnitsCompatible(
	const std::string & strSourceUnit,
	const std::string & strTargetUnit
);

///	<summary>
///		Extract the value and unit of this string.
///	</summary>
bool ExtractValueUnit(
	const std::string & strString,
	double & dValue,
	std::string & strUnit
);

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Retrieve the contents of this Object as a list of strings.
///	</summary>
std::string ArgumentToStringVector(
	const ObjectRegistry & objreg,
	const std::string & strArgument,
	std::vector<std::string> & vecStringList
);

///////////////////////////////////////////////////////////////////////////////

#endif

