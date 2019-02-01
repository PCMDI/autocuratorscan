///////////////////////////////////////////////////////////////////////////////
///
///	\file    Object.cpp
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

#include "Object.h"
#include "Announce.h"

#include <cstdlib>
#include <cmath>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// ObjectRegistry
///////////////////////////////////////////////////////////////////////////////

ObjectRegistry::~ObjectRegistry() {
	ObjectMap::iterator iter = m_mapObjects.begin();
	for (; iter != m_mapObjects.end(); iter++) {
		delete(iter->second);
	}
}

///////////////////////////////////////////////////////////////////////////////

Object * ObjectRegistry::GetObject(
	const std::string & strName
) const {

	ObjectMap::const_iterator iter = m_mapObjects.find(strName);
	if (iter == m_mapObjects.end()) {
		return NULL;
	} else {
		return (iter->second);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool ObjectRegistry::Create(
	const ObjectType & objtype,
	const std::string & strName,
	const std::string & strValue
) {
	// String type on RHS
	if (objtype == ObjectType_String) {
		return Assign(
			strName,
			new StringObject(
				"",
				strValue));

	// Integer type on RHS
	} else if (objtype == ObjectType_Integer) {
		return Assign(
			strName,
			new IntegerObject(
				"",
				atoi(strValue.c_str())));

	// Floating point type on RHS
	} else if (objtype == ObjectType_FloatingPoint) {
		return Assign(
			strName,
			new FloatingPointObject(
				"",
				atof(strValue.c_str())));

	// Object type on RHS
	} else if (objtype == ObjectType_Token) {
		Object * pobj = GetObject(strValue);
		if (pobj != NULL) {
			Object * pobjDuplicate = pobj->Duplicate(strName, *this);
			if (pobjDuplicate != NULL) {
				return true;
			} else {
				_EXCEPTIONT("Unknown error");
			}
		} else {
			return false;
		}
	}

	// Invalid type
	return false;
}

///////////////////////////////////////////////////////////////////////////////

std::string ObjectRegistry::Remove(
	const std::string & strName
) {
	// Check if this Object already exists
	ObjectMap::iterator iter = m_mapObjects.find(strName);
	if (iter == m_mapObjects.end()) {
		_EXCEPTION1("Object \"%s\" not found in registry", strName.c_str());
	}

	if (iter->second->m_nLocks != 0) {
		return std::string("ERROR: Cannot delete locked object \"")
			+ strName + std::string("\"");
	}

	Announce("DELETE %s\n", strName.c_str());

	// Notify object of impending deletion
	iter->second->PrepareDelete();

	// Remove all children of this Object
	for (size_t i = 0; i < iter->second->m_vecChildren.size(); i++) {
		std::string strError =
			Remove(iter->second->m_vecChildren[i]->m_strName);

		if (strError != "") {
			return strError;
		}
	}

	// Remove this Object
	delete (iter->second);
	m_mapObjects.erase(iter);

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

bool ObjectRegistry::Assign(
	const std::string & strName,
	Object * pObject
) {
	if (strName == "__TEMPNAME__") {
		_EXCEPTIONT("Token \"__TEMPNAME__\" is restricted");
	}
	printf("ASSIGN %s\n", strName.c_str());

	// Check if this Object already exists
	ObjectMap::const_iterator iter = m_mapObjects.find(strName);
	if (iter != m_mapObjects.end()) {
		std::string strError = Remove(strName);
		if (strError != "") {
			Announce(strError.c_str());
			return false;
		}
	}

	// Assign the Object its name
	if (pObject->m_strName == "") {
		pObject->m_strName = strName;

	} else if (
		(pObject->m_strName.length() == 12) &&
	   	(pObject->m_strName.substr(0,12) == "__TEMPNAME__")
	) {
		pObject->m_strName = strName;

	} else {
		_EXCEPTION1("Invalid name in assignment \"%s\"", pObject->m_strName.c_str());
	}

	// Add the Object to its parent
	std::string strParent;
	for (int i = strName.length()-1; i >= 0; i--) {
		if (strName[i] == '.') {
			strParent = strName.substr(0,i);
			break;
		}
	}

	// Put a pointer to the Object into the parent's array
	if (strParent != "") {
		ObjectMap::const_iterator iterParent = m_mapObjects.find(strParent);
		if (iterParent == m_mapObjects.end()) {
			return false;
		}

		//printf("PARENT %s\n", strParent.c_str());
		bool fSuccess = iterParent->second->AddChild(pObject);
		if (!fSuccess) {
			return false;
		}
	}

	// Insert the Object into the ObjectRegistry
	m_mapObjects.insert(
		ObjectMap::value_type(strName, pObject));

	// Insert any children of this object to the ObjectRegistry
	for (int k = 0; k < pObject->m_vecChildrenUnassigned.size(); k++) {
		if ((pObject->m_vecChildrenUnassigned[k]->m_strName.length() >= 12) &&
	   		(pObject->m_vecChildrenUnassigned[k]->m_strName.substr(0,12) == "__TEMPNAME__")
		) {
			std::string strNewChildName =
				strName + pObject->m_vecChildrenUnassigned[k]->m_strName.substr(12, std::string::npos);
			pObject->m_vecChildrenUnassigned[k]->m_strName = "";
			Assign(strNewChildName, pObject->m_vecChildrenUnassigned[k]);

		} else {
			_EXCEPTIONT("Logic error");
		}
	}
	pObject->m_vecChildrenUnassigned.clear();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// ObjectConstructor
///////////////////////////////////////////////////////////////////////////////

std::string ObjectConstructor::Call(
	const ObjectRegistry & objreg,
	const std::vector<std::string> & vecCommandLine,
	const std::vector<ObjectType> & vecCommandLineType,
	Object ** ppReturn
) {
	if (ppReturn != NULL) {
		Object * pobj = new Object("");
		if (pobj == NULL) {
			_EXCEPTIONT("Unable to initialize Object");
		}

		// Set the return value
		(*ppReturn) = pobj;
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////
// Object
///////////////////////////////////////////////////////////////////////////////

Object * Object::_Duplicate(
	Object * pobjDuplicate,
	ObjectRegistry & objreg
) const {
	bool fSuccess = objreg.Assign(pobjDuplicate->m_strName, pobjDuplicate);
	if (!fSuccess) {
		_EXCEPTIONT("Failed to insert Object into registry");
	}

	const int nOriginalNameLength = m_strName.length();

	const int nNewNameLength = pobjDuplicate->m_strName.length();

	for (size_t i = 0; i < m_vecChildren.size(); i++) {

		//printf("ORIGINAL CHILD %s\n", (*iter)->m_strName.c_str());
		std::string strNewChildName =
			pobjDuplicate->m_strName
			+ m_vecChildren[i]->m_strName.substr(
				nOriginalNameLength, std::string::npos);

		if (strNewChildName[nNewNameLength] != '.') {
			_EXCEPTIONT("Logic error: Invalid child");
		}

		m_vecChildren[i]->Duplicate(strNewChildName, objreg);
	}

	return (pobjDuplicate);
}

///////////////////////////////////////////////////////////////////////////////
// StringObject
///////////////////////////////////////////////////////////////////////////////

bool StringObject::ToUnit(
	const std::string & strTargetUnit,
	double * dValueOut,
	bool fIsDelta
) {
	// Check argument
	if (dValueOut == NULL) {
		_EXCEPTIONT("Invalid pointer to double on return");
	}

	return
		StringToValueUnit(
			m_strValue,
			strTargetUnit,
			(*dValueOut),
			fIsDelta);
}

///////////////////////////////////////////////////////////////////////////////
// ListConstructor
///////////////////////////////////////////////////////////////////////////////

std::string ListObjectSpanConstructor::Call(
	const ObjectRegistry & objreg,
	const std::vector<std::string> & vecCommandLine,
	const std::vector<ObjectType> & vecCommandLineType,
	Object ** ppReturn
) {
	if (ppReturn != NULL) {
		ListObject * pobj = new ListObject("__TEMPNAME__");
		if (pobj == NULL) {
			_EXCEPTIONT("Unable to initialize ListObject");
		}

		if (vecCommandLine.size() < 2) {
			return std::string("ERROR: list_span must have at least two arguments");
		}

		// Loop through all elements of span
		for (int k = 0; k < vecCommandLine.size()-1; k++) {
			int nColons = 0;
			int iLastColon = (-1);
			std::vector<double> dValue;
			for (int i = 0; i < vecCommandLine[k].length(); i++) {
				if (vecCommandLine[k][i] == ':') {
					nColons++;
					dValue.push_back(
						atof(vecCommandLine[k].substr(
							iLastColon+1, i-iLastColon-1).c_str()));
					iLastColon = i;
				}
			}
			dValue.push_back(atof(
				vecCommandLine[k].substr(
					iLastColon+1, std::string::npos).c_str()));

			if ((nColons != 2) || (dValue.size() != 3)) {
				return std::string("ERROR: list_span argument must consist of three values, "
					"separated by colons");
			}
			if (dValue[2] == 0.0) {
				return std::string("ERROR: Infinite number of items in list_span");
			}

			double dSign = 1.0;
			if (dValue[2] < 0.0) {
				dSign = -1.0;
			}

			std::string strUnits = vecCommandLine[vecCommandLine.size()-1];

			double dTarget = (1.0+dSign*1.0e-12)*dValue[1]*dSign;
			int ix = pobj->Count();
			for (double d = dValue[0]; d*dSign <= dTarget; d += dValue[2]) {
				pobj->PushBack(pobj->ChildName(ix));
				pobj->AddUnassignedChild(
					new StringObject(
						pobj->ChildName(ix),
						std::to_string(d) + strUnits));
				ix++;
			}
		}

		// Set the return value
		(*ppReturn) = pobj;
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////////

bool ConvertUnits(
	double dValue,
	const std::string & strUnit,
	double & dValueOut,
	const std::string & strTargetUnit,
	bool fIsDelta
) {
	// Unit is equal to TargetUnit
	if (strUnit == strTargetUnit) {
		dValueOut = dValue;

	// Perform unit conversion from great circle distance (degrees)
	} else if ((strUnit == "deg") || (strUnit == "degrees_north") || (strUnit == "degrees_east")) {
		if ((strTargetUnit == "deg") ||
		    (strTargetUnit == "degrees_north") ||
		    (strTargetUnit == "degrees_east")
		) {
			dValueOut = dValue;

		} else if (strTargetUnit == "rad") {
			dValueOut = dValue * M_PI / 180.0;

		} else if (strTargetUnit == "m") {
			dValueOut = 6.37122e6 * dValue * M_PI / 180.0;

		} else if (strTargetUnit == "km") {
			dValueOut = 6.37122e3 * dValue * M_PI / 180.0;

		} else {
			return false;
		}

	// Perform unit conversion from great circle distance (radians)
	} else if (strUnit == "rad") {
		if ((strTargetUnit == "deg") ||
		    (strTargetUnit == "degrees_north") ||
		    (strTargetUnit == "degrees_east")
		) {
			dValueOut = 180.0 / M_PI * dValue;

		} else if (strTargetUnit == "m") {
			dValueOut = 6.37122e6 * dValue;

		} else if (strTargetUnit == "km") {
			dValueOut = 6.37122e3 * dValue;

		} else {
			return false;
		}

	// Perform unit conversion from great circle distance (meters)
	// or altitude (meters)
	} else if (strUnit == "m") {
		if ((strTargetUnit == "deg") ||
		    (strTargetUnit == "degrees_north") ||
		    (strTargetUnit == "degrees_east")
		) {
			dValueOut = 180.0 / M_PI * dValue / 6.37122e6;

		} else if (strTargetUnit == "rad") {
			dValueOut = dValue / 6.37122e6;

		} else if (strTargetUnit == "km") {
			dValueOut = dValue / 1000.0;

		} else if (strTargetUnit == "m2/s2") {
			dValueOut = dValue * 9.80616;

		} else {
			return false;
		}

	// Perform unit conversion from great circle distance (kilometers)
	} else if (strUnit == "km") {
		if ((strTargetUnit == "deg") ||
		    (strTargetUnit == "degrees_north") ||
		    (strTargetUnit == "degrees_east")
		) {
			dValueOut = 180.0 / M_PI * dValue / 6.37122e3;

		} else if (strTargetUnit == "rad") {
			dValueOut = dValue / 6.37122e3;

		} else if (strTargetUnit == "m") {
			dValueOut = dValue * 1000.0;

		} else if (strTargetUnit == "m2/s2") {
			dValueOut = dValue * 1000.0 * 9.80616;

		} else {
			return false;
		}

	// Perform unit conversion from temperature (K)
	} else if (strUnit == "K") {
		if (strTargetUnit == "degC") {
			if (fIsDelta) {
				dValueOut = dValue;
			} else {
				dValueOut = dValue - 273.15;
			}

		} else {
			return false;
		}

	// Perform unit conversion from temperature (degC)
	} else if (strUnit == "degC") {
		if (strTargetUnit == "K") {
			if (fIsDelta) {
				dValueOut = dValue;
			} else {
				dValueOut = dValue + 273.15;
			}

		} else {
			return false;
		}

	// Perform unit conversion from pressure (Pa)
	} else if (strUnit == "Pa") {
		if ((strTargetUnit == "hPa") ||
		    (strTargetUnit == "mb") ||
		    (strTargetUnit == "mbar")
		) {
			dValueOut = dValue / 100.0;

		} else if (strTargetUnit == "atm") {
			dValueOut = dValue / 101325.0;

		} else {
			return false;
		}

	// Perform unit conversion from pressure (hPa,mb,mbar)
	} else if ((strUnit == "hPa") || (strUnit == "mb") || (strUnit == "mbar")) {
		if (strTargetUnit == "Pa") {
			dValueOut = dValue * 100.0;

		} else if (
		    (strTargetUnit == "hPa") ||
		    (strTargetUnit == "mb") ||
		    (strTargetUnit == "mbar")
		) {
			dValueOut = dValue;

		} else if (strTargetUnit == "atm") {
			dValueOut = dValue / 1013.25;

		} else {
			return false;
		}

	// Perform unit conversion from pressure (atm)
	} else if (strUnit == "atm") {
		if (strTargetUnit == "Pa") {
			dValueOut = dValue * 101325.0;

		} else if (
		    (strTargetUnit == "hPa") ||
		    (strTargetUnit == "mb") ||
		    (strTargetUnit == "mbar")
		) {
			dValueOut = dValue * 1013.25;

		} else {
			return false;
		}

	// Perform unit conversion from geopotential (m2/s2)
	} else if (strUnit == "m2/s2") {
		if (strTargetUnit == "m") {
			dValueOut = dValue / 9.80616;

		} else if (strUnit == "km") {
			dValueOut = dValue / 9.80616 / 1000.0;

		} else {
			return false;
		}

	} else {
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool StringToValueUnit(
	const std::string & strString,
	const std::string & strTargetUnit,
	double & dValueOut,
	bool fIsDelta
) {

	// Extract the value and unit
	double dValue;
	std::string strUnit;

	bool fParseSuccess =
		ExtractValueUnit(
			strString,
			dValue,
			strUnit);

	if (!fParseSuccess) {
		return false;
	}

	return ConvertUnits(
		dValue,
		strUnit,
		dValueOut,
		strTargetUnit,
		fIsDelta);
}

///////////////////////////////////////////////////////////////////////////////

bool AreUnitsCompatible(
	const std::string & strSourceUnit,
	const std::string & strTargetUnit
) {
	std::string strString = std::string("0") + strSourceUnit;

	double dValue;

	return StringToValueUnit(
		strString,
		strTargetUnit,
		dValue,
		false);
}

///////////////////////////////////////////////////////////////////////////////

bool ExtractValueUnit(
	const std::string & strString,
	double & dValue,
	std::string & strUnit
) {

	// Extract the value and unit from this String
	enum ParseMode {
		ParseMode_WS,
		ParseMode_Number,
		ParseMode_Unit
	};

	ParseMode mode = ParseMode_WS;
	ParseMode modeNext = ParseMode_Number;

	bool fHasPeriod = false;

	std::string strNumber;

	int iPos = 0;
	for (;;) {
		if (iPos >= strString.length()) {
			break;
		}

		// Whitespace
		if (mode == ParseMode_WS) {
			if ((strString[iPos] == ' ') ||
			    (strString[iPos] == '\t')
			) {
				iPos++;
			} else {
				mode = modeNext;
			}

		// Number
		} else if (mode == ParseMode_Number) {
			if ((strString[iPos] >= '0') && (strString[iPos] <= '9')) {
				strNumber += strString[iPos];
				iPos++;

			} else if (strString[iPos] == '.') {
				if (fHasPeriod) {
					return false;
				} else {
					strNumber += strString[iPos];
					fHasPeriod = true;
					iPos++;
				}

			} else if (strString[iPos] == '-') {
				if (strNumber.length() != 0) {
					return false;
				} else {
					strNumber += strString[iPos];
					iPos++;
				}

			} else if (
				((strString[iPos] >= 'a') && (strString[iPos] <= 'z')) ||
				((strString[iPos] >= 'A') && (strString[iPos] <= 'Z'))
			) {
				mode = ParseMode_Unit;

			} else if (
				(strString[iPos] == ' ') ||
			    (strString[iPos] == '\t')
			) {
				mode = ParseMode_WS;
				modeNext = ParseMode_Unit;

			} else {
				return false;
			}

		// Unit
		} else if (mode == ParseMode_Unit) {
			if (((strString[iPos] >= 'a') && (strString[iPos] <= 'z')) ||
				((strString[iPos] >= 'A') && (strString[iPos] <= 'Z'))
			) {
				strUnit += strString[iPos];
				iPos++;

			} else if (
				(strString[iPos] == ' ') ||
			    (strString[iPos] == '\t')
			) {
				break;

			} else {
				return false;
			}

		// Invalid mode
		} else {
			_EXCEPTIONT("Invalid mode");
		}
	}

	if (strNumber.length() == 0) {
		return false;
	}

	// Value
	dValue = atof(strNumber.c_str());

	return true;
}

///////////////////////////////////////////////////////////////////////////////

std::string ArgumentToStringVector(
	const ObjectRegistry & objreg,
	const std::string & strArgument,
	std::vector<std::string> & vecStringList
) {
	ListObject * pobjVariableList =
		dynamic_cast<ListObject *>(
			objreg.GetObject(strArgument));

	if (pobjVariableList != NULL) {
		for (size_t i = 0; i < pobjVariableList->Count(); i++) {
			StringObject * pobjVariableString =
				dynamic_cast<StringObject *>(
					objreg.GetObject(pobjVariableList->ChildName(i)));

			if (pobjVariableString == NULL) {
				return std::string("ERROR: Argument must be a list of string objects");
			}

			vecStringList.push_back(pobjVariableString->Value());
		}

	} else {
		StringObject * pobjVariableString =
			dynamic_cast<StringObject *>(
				objreg.GetObject(strArgument));
		if (pobjVariableString == NULL) {
			vecStringList.push_back(strArgument);
		} else {
			vecStringList.push_back(pobjVariableString->Value());
		}
	}
	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

