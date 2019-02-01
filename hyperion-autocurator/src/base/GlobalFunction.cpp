///////////////////////////////////////////////////////////////////////////////
///
///	\file    GlobalFunction.cpp
///	\author  Paul Ullrich
///	\version March 14, 2017
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

#include "GlobalFunction.h"

#include <cstdlib>

///////////////////////////////////////////////////////////////////////////////
// GlobalFunctionRegistry
///////////////////////////////////////////////////////////////////////////////

GlobalFunctionRegistry::~GlobalFunctionRegistry() {
	GlobalFunctionMap::iterator iter = m_mapGlobalFunctions.begin();
	for (; iter != m_mapGlobalFunctions.end(); iter++) {
		delete(iter->second);
	}
}

///////////////////////////////////////////////////////////////////////////////

GlobalFunction * GlobalFunctionRegistry::GetGlobalFunction(
	const std::string & strName
) const {

	GlobalFunctionMap::const_iterator iter =
		m_mapGlobalFunctions.find(strName);

	if (iter == m_mapGlobalFunctions.end()) {
		return NULL;
	} else {
		return (iter->second);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool GlobalFunctionRegistry::Assign(
	const std::string & strName,
	GlobalFunction * pGlobalFunction
) {
	//printf("ASSIGN %s\n", strName.c_str());

	// Add the GlobalFunction to its parent
	std::string strParent;
	for (int i = strName.length()-1; i >= 0; i--) {
		if (strName[i] == '.') {
			_EXCEPTION1("Global function \"%s\" cannot contain period character",
				strName.c_str());
		}
	}

	// Check if this GlobalFunction already exists
	GlobalFunctionMap::const_iterator iter = m_mapGlobalFunctions.find(strName);
	if (iter != m_mapGlobalFunctions.end()) {
		_EXCEPTION1("Duplicate global functions \"%s\"",
			strName.c_str());
	}

	// Assign the GlobalFunction into the GlobalFunctionRegistry
	m_mapGlobalFunctions.insert(
		GlobalFunctionMap::value_type(strName, pGlobalFunction));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// GlobalFunction
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

