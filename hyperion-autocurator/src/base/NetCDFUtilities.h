///////////////////////////////////////////////////////////////////////////////
///
///	\file    NetCDFUtilities.h
///	\author  Paul Ullrich
///	\version August 14, 2014
///
///	<remarks>
///		Copyright 2000-2014 Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#ifndef _NETCDFUTILITIES_H_
#define _NETCDFUTILITIES_H_

#include <string>
#include "netcdfcpp.h"

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Determine if the given string is a valid NetCDF variable name.
///	</summary>
bool IsValidNetCDFVariableName(
	const std::string & strVariableName
);

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Get a string representation of the given NcType.
///	</summary>
std::string NcTypeToString(
	NcType nctype
);

////////////////////////////////////////////////////////////////////////////////

void CopyNcFileAttributes(
	NcFile * fileIn,
	NcFile * fileOut
);

////////////////////////////////////////////////////////////////////////////////

void CopyNcVarAttributes(
	NcVar * varIn,
	NcVar * varOut
);

////////////////////////////////////////////////////////////////////////////////

void CopyNcVar(
	NcFile & ncIn,
	NcFile & ncOut,
	const std::string & strVarName,
	bool fCopyAttributes = true
);

////////////////////////////////////////////////////////////////////////////////

#endif

