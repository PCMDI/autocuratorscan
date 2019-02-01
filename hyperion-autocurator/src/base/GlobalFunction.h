///////////////////////////////////////////////////////////////////////////////
///
///	\file    GlobalFunction.h
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

#ifndef _GLOBALFUNCTION_H_
#define _GLOBALFUNCTION_H_

#include "Exception.h"
#include "ObjectType.h"

#include <string>
#include <set>
#include <vector>
#include <map>

///////////////////////////////////////////////////////////////////////////////

class ObjectRegistry;

class Object;

class GlobalFunction;

typedef std::set<GlobalFunction *> GlobalFunctionChildrenSet;

typedef std::map<std::string, GlobalFunction *> GlobalFunctionMap;

typedef int GlobalFunctionIndex;

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for registering GlobalFunctions.
///	</summary>
class GlobalFunctionRegistry {

public:
	///	<summary>
	///		Destructor.
	///	</summary>
	~GlobalFunctionRegistry();

public:
	///	<summary>
	///		Get the GlobalFunction with the specified name.
	///	</summary>
	GlobalFunction * GetGlobalFunction(
		const std::string & strName
	) const;

	///	<summary>
	///		Assign the GlobalFunction with the specified name.
	///	</summary>
	///	<returns>
	///		true if insertion is successful.  false if parent GlobalFunction could
	///		not be found in the GlobalFunctionRegistry.
	///	</returns>
	bool Assign(
		const std::string & strName,
		GlobalFunction * pGlobalFunction
	);

private:
	///	<summary>
	///		Map from GlobalFunction name to GlobalFunction instance.
	///	</summary>
	GlobalFunctionMap m_mapGlobalFunctions;
};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for representing objects.
///	</summary>
class GlobalFunction {

friend class GlobalFunctionRegistry;

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	GlobalFunction(const std::string & strName) :
		m_strName(strName)
	{ }

	///	<summary>
	///		Virtual destructor.
	///	</summary>
	virtual ~GlobalFunction() {
	}

	///	<summary>
	///		Call a member function of this GlobalFunction.
	///	</summary>
	virtual std::string Call(
		const ObjectRegistry & objreg,
		const std::vector<std::string> & vecCommandLine,
		const std::vector<ObjectType> & vecCommandLineType,
		Object ** ppReturn
	) {
		return std::string("");
	}

protected:
	///	<summary>
	///		Name of the GlobalFunction.
	///	</summary>
	std::string m_strName;
};

///////////////////////////////////////////////////////////////////////////////

#endif

