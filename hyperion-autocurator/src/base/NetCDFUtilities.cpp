///////////////////////////////////////////////////////////////////////////////
///
///	\file    NetCDFUtilities.cpp
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

#include "NetCDFUtilities.h"
#include "Exception.h"
#include "DataArray1D.h"
#include "netcdfcpp.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////

bool IsValidNetCDFVariableName(
	const std::string & strVar
) {
	// Valid interior characters:
	//   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+_.@"
	// Valid first characters:
	//   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"

	if (strVar.length() == 0) {
		return false;
	}
	if (((strVar[0] >= 'a') && (strVar[0] <= 'z')) ||
	    ((strVar[0] >= 'A') && (strVar[0] <= 'Z')) ||
	    ((strVar[0] >= '0') && (strVar[0] <= '9')) ||
	    (strVar[0] == '_')
	) {
	} else {
		return false;
	}
	for (int i = 1; i < strVar.length(); i++) {
		if (((strVar[i] >= 'a') && (strVar[i] <= 'z')) ||
		    ((strVar[i] >= 'A') && (strVar[i] <= 'Z')) ||
		    ((strVar[i] >= '0') && (strVar[i] <= '9')) ||
		    (strVar[i] == '_') ||
		    (strVar[i] == '+') ||
		    (strVar[i] == '-') ||
		    (strVar[i] == '.') ||
		    (strVar[i] == '@')
		) {
		} else {
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

std::string NcTypeToString(
	NcType nctype
) {
	switch(nctype) {
		case ncNoType:
			return std::string("Unspecified");

		case ncByte:
			return std::string("Byte");

		case ncChar:
			return std::string("Char");

		case ncShort:
			return std::string("Short");

		case ncInt:
			return std::string("Int");

		case ncFloat:
			return std::string("Float");

		case ncDouble:
			return std::string("Double");

		case ncUByte:
			return std::string("UByte");

		case ncUShort:
			return std::string("UShort");

		case ncUInt:
			return std::string("UInt");

		case ncInt64:
			return std::string("Int64");

		case ncUInt64:
			return std::string("UInt64");
		
		case ncString:
			return std::string("String");

		default:
			_EXCEPTIONT("Invalid NcType");
	};
}

////////////////////////////////////////////////////////////////////////////////

void CopyNcFileAttributes(
	NcFile * fileIn,
	NcFile * fileOut
) {
	for (int a = 0; a < fileIn->num_atts(); a++) {
		NcAtt * att = fileIn->get_att(a);
		long num_vals = att->num_vals();

		NcValues * pValues = att->values();

		if (att->type() == ncByte) {
			fileOut->add_att(att->name(), num_vals,
				(const ncbyte*)(pValues->base()));

		} else if (att->type() == ncChar) {
			fileOut->add_att(att->name(), num_vals,
				(const char*)(pValues->base()));

		} else if (att->type() == ncShort) {
			fileOut->add_att(att->name(), num_vals,
				(const short*)(pValues->base()));

		} else if (att->type() == ncInt) {
			fileOut->add_att(att->name(), num_vals,
				(const int*)(pValues->base()));

		} else if (att->type() == ncFloat) {
			fileOut->add_att(att->name(), num_vals,
				(const float*)(pValues->base()));

		} else if (att->type() == ncDouble) {
			fileOut->add_att(att->name(), num_vals,
				(const double*)(pValues->base()));

		} else {
			_EXCEPTIONT("Invalid attribute type");
		}

		delete pValues;
	}
}

////////////////////////////////////////////////////////////////////////////////

void CopyNcVarAttributes(
	NcVar * varIn,
	NcVar * varOut
) {
	for (int a = 0; a < varIn->num_atts(); a++) {
		NcAtt * att = varIn->get_att(a);
		long num_vals = att->num_vals();

		NcValues * pValues = att->values();

		if (att->type() == ncByte) {
			varOut->add_att(att->name(), num_vals,
				(const ncbyte*)(pValues->base()));

		} else if (att->type() == ncChar) {
			varOut->add_att(att->name(), num_vals,
				(const char*)(pValues->base()));

		} else if (att->type() == ncShort) {
			varOut->add_att(att->name(), num_vals,
				(const short*)(pValues->base()));

		} else if (att->type() == ncInt) {
			varOut->add_att(att->name(), num_vals,
				(const int*)(pValues->base()));

		} else if (att->type() == ncFloat) {
			varOut->add_att(att->name(), num_vals,
				(const float*)(pValues->base()));

		} else if (att->type() == ncDouble) {
			varOut->add_att(att->name(), num_vals,
				(const double*)(pValues->base()));

		} else {
			_EXCEPTIONT("Invalid attribute type");
		}

		delete pValues;
	}
}

////////////////////////////////////////////////////////////////////////////////

void CopyNcVar(
	NcFile & ncIn,
	NcFile & ncOut,
	const std::string & strVarName,
	bool fCopyAttributes
) {
	if (!ncIn.is_valid()) {
		_EXCEPTIONT("Invalid input file specified");
	}
	if (!ncOut.is_valid()) {
		_EXCEPTIONT("Invalid output file specified");
	}
	NcVar * var = ncIn.get_var(strVarName.c_str());
	if (var == NULL) {
		_EXCEPTION1("NetCDF file does not contain variable \"%s\"",
			strVarName.c_str());
	}

	NcVar * varOut;

	std::vector<NcDim *> dimOut;
	dimOut.resize(var->num_dims());

	std::vector<long> counts;
	counts.resize(var->num_dims());

	long nDataSize = 1;

	for (int d = 0; d < var->num_dims(); d++) {
		NcDim * dimA = var->get_dim(d);

		dimOut[d] = ncOut.get_dim(dimA->name());

		if (dimOut[d] == NULL) {
			if (dimA->is_unlimited()) {
				dimOut[d] = ncOut.add_dim(dimA->name());
			} else {
				dimOut[d] = ncOut.add_dim(dimA->name(), dimA->size());
			}

			if (dimOut[d] == NULL) {
				_EXCEPTION2("Failed to add dimension \"%s\" (%i) to file",
					dimA->name(), dimA->size());
			}
		}
		if (dimOut[d]->size() != dimA->size()) {
			if (dimA->is_unlimited() && !dimOut[d]->is_unlimited()) {
				_EXCEPTION2("Mismatch between input file dimension \"%s\" and "
					"output file dimension (UNLIMITED / %i)",
					dimA->name(), dimOut[d]->size());
			} else if (!dimA->is_unlimited() && dimOut[d]->is_unlimited()) {
				_EXCEPTION2("Mismatch between input file dimension \"%s\" and "
					"output file dimension (%i / UNLIMITED)",
					dimA->name(), dimA->size());
			} else if (!dimA->is_unlimited() && !dimOut[d]->is_unlimited()) {
				_EXCEPTION3("Mismatch between input file dimension \"%s\" and "
					"output file dimension (%i / %i)",
					dimA->name(), dimA->size(), dimOut[d]->size());
			}
		}

		counts[d] = dimA->size();
		nDataSize *= counts[d];
	}

	// ncByte / ncChar type
	if ((var->type() == ncByte) || (var->type() == ncChar)) {
		DataArray1D<char> data(nDataSize);

		varOut =
			ncOut.add_var(
				var->name(), var->type(),
				dimOut.size(), (const NcDim**)&(dimOut[0]));

		if (varOut == NULL) {
			_EXCEPTION1("Cannot create variable \"%s\"", var->name());
		}

		var->get(&(data[0]), &(counts[0]));
		varOut->put(&(data[0]), &(counts[0]));
	}

	// ncShort type
	if (var->type() == ncShort) {
		DataArray1D<short> data(nDataSize);

		varOut =
			ncOut.add_var(
				var->name(), var->type(),
				dimOut.size(), (const NcDim**)&(dimOut[0]));

		if (varOut == NULL) {
			_EXCEPTION1("Cannot create variable \"%s\"", var->name());
		}

		var->get(&(data[0]), &(counts[0]));
		varOut->put(&(data[0]), &(counts[0]));
	}

	// ncInt type
	if (var->type() == ncInt) {
		DataArray1D<int> data(nDataSize);

		varOut =
			ncOut.add_var(
				var->name(), var->type(),
				dimOut.size(), (const NcDim**)&(dimOut[0]));

		if (varOut == NULL) {
			_EXCEPTION1("Cannot create variable \"%s\"", var->name());
		}

		var->get(&(data[0]), &(counts[0]));
		varOut->put(&(data[0]), &(counts[0]));
	}

	// ncFloat type
	if (var->type() == ncFloat) {
		DataArray1D<float> data(nDataSize);

		varOut =
			ncOut.add_var(
				var->name(), var->type(),
				dimOut.size(), (const NcDim**)&(dimOut[0]));

		if (varOut == NULL) {
			_EXCEPTION1("Cannot create variable \"%s\"", var->name());
		}

		var->get(&(data[0]), &(counts[0]));
		varOut->put(&(data[0]), &(counts[0]));
	}


	// ncDouble type
	if (var->type() == ncDouble) {
		DataArray1D<double> data(nDataSize);

		varOut =
			ncOut.add_var(
				var->name(), var->type(),
				dimOut.size(), (const NcDim**)&(dimOut[0]));

		if (varOut == NULL) {
			_EXCEPTION1("Cannot create variable \"%s\"", var->name());
		}

		var->get(&(data[0]), &(counts[0]));
		varOut->put(&(data[0]), &(counts[0]));
	}

	// Check output variable exists
	if (varOut == NULL) {
		_EXCEPTION1("Unable to create output variable \"%s\"",
			var->name());
	}

	// Copy attributes
	if (fCopyAttributes) {
		CopyNcVarAttributes(var, varOut);
	}
}

////////////////////////////////////////////////////////////////////////////////


