///////////////////////////////////////////////////////////////////////////////
///
///	\file    ObjectType.h
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

#ifndef _OBJECTTYPE_H_
#define _OBJECTTYPE_H_

///////////////////////////////////////////////////////////////////////////////

typedef int ObjectType;

static const ObjectType ObjectType_Op = 0;

static const ObjectType ObjectType_Token = 1;

static const ObjectType ObjectType_String = 2;

static const ObjectType ObjectType_Integer = 3;

static const ObjectType ObjectType_FloatingPoint = 4;

///////////////////////////////////////////////////////////////////////////////

#endif

