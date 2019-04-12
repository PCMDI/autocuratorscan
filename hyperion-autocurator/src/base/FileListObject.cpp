///////////////////////////////////////////////////////////////////////////////
///
///	\file    FileListObject.cpp
///	\author  Paul Ullrich
///	\version March 10, 2017
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

#include "FileListObject.h"
#include "STLStringHelper.h"
#include "DataArray1D.h"
#include "DataArray2D.h"
#include "netcdfcpp.h"
#include "NetCDFUtilities.h"
#include "../contrib/tinyxml2.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <iomanip>

#if defined(HYPERION_MPIOMP)
#include <mpi.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// DataObjectInfo
///////////////////////////////////////////////////////////////////////////////

std::string DataObjectInfo::FromNcFile(
	NcFile * ncfile,
	bool fCheckConsistency,
	const std::string & strFilename
) {
	// Get attributes, if available
	for (int a = 0; a < ncfile->num_atts(); a++) {
		NcAtt * att = ncfile->get_att(a);
		std::string strAttName = att->name();
		if (strAttName == "units") {
			continue;
		}

		// Define new value of this attribute
		if (!fCheckConsistency) {
			std::string strAttNameTemp = strAttName;
			STLStringHelper::ToLower(strAttNameTemp);

			if ((strAttNameTemp == "conventions") ||
			    (strAttNameTemp == "version") ||
			    (strAttNameTemp == "history")
			) {
				m_mapKeyAttributes.insert(
					AttributeMap::value_type(
						strAttName, att->as_string(0)));
			} else {
				m_mapOtherAttributes.insert(
					AttributeMap::value_type(
						strAttName, att->as_string(0)));
			}

		// Check for consistency across files
		} else {
			AttributeMap::const_iterator iterAttKey =
				m_mapKeyAttributes.find(strAttName);
			AttributeMap::const_iterator iterAttOther =
				m_mapOtherAttributes.find(strAttName);

			if (iterAttKey != m_mapKeyAttributes.end()) {
				if (iterAttKey->second != att->as_string(0)) {
					return std::string("ERROR: NetCDF file \"") + strFilename
						+ std::string("\" has inconsistent value of \"")
						+ strAttName + std::string("\" across files");
				}
			}
			if (iterAttOther != m_mapOtherAttributes.end()) {
				if (iterAttOther->second != att->as_string(0)) {
					return std::string("ERROR: NetCDF file \"") + strFilename
						+ std::string("\" has inconsistent value of \"")
						+ strAttName + std::string("\" across files");
				}
			}
			if ((iterAttKey == m_mapKeyAttributes.end()) &&
			    (iterAttOther == m_mapOtherAttributes.end())
			) {
				return std::string("ERROR: NetCDF file \"") + strFilename
					+ std::string("\" has inconsistent appearance of attribute \"")
					+ strAttName + std::string("\" across files");
			}
		}
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string DataObjectInfo::FromNcVar(
	NcVar * var,
	bool fCheckConsistency
) {
	// Get name, if available
	std::string strName = var->name();
	if (!fCheckConsistency) {
		m_strName = strName;
	} else if (strName != m_strName) {
		_EXCEPTION2("Calling DataObjectInfo::FromNcVar with mismatched "
			"variable names: \"%s\" \"%s\"",
			strName.c_str(), m_strName.c_str());
	}

	// Get type, if available
	NcType nctype = var->type();
	if (!fCheckConsistency) {
		m_nctype = nctype;
	} else if (nctype != m_nctype) {
		return std::string("ERROR: Variable \"") + strName
			+ std::string("\" has inconsistent type across files");
	}

	// Get units, if available
	std::string strUnits;
	NcAtt * attUnits = var->get_att("units");
	if (attUnits != NULL) {
		strUnits = attUnits->as_string(0);
	}
	if (!fCheckConsistency) {
		m_strUnits = strUnits;
	} else if (strUnits != m_strUnits) {
		return std::string("ERROR: Variable \"") + strName
			+ std::string("\" has inconsistent units across files");
	}

	// Get attributes, if available
	for (int a = 0; a < var->num_atts(); a++) {
		NcAtt * att = var->get_att(a);
		std::string strAttName = att->name();
		if (strAttName == "units") {
			continue;
		}

		// Define new value of this attribute
		if (!fCheckConsistency) {
			if ((strAttName == "missing_value") ||
			    (strAttName == "comments") ||
			    (strAttName == "long_name") ||
			    (strAttName == "grid_name") ||
			    (strAttName == "grid_type")
			) {
				m_mapKeyAttributes.insert(
					AttributeMap::value_type(
						strAttName, att->as_string(0)));
			} else {
				m_mapOtherAttributes.insert(
					AttributeMap::value_type(
						strAttName, att->as_string(0)));
			}

		// Check for consistency across files
		} else {
			AttributeMap::const_iterator iterAttKey =
				m_mapKeyAttributes.find(strAttName);
			AttributeMap::const_iterator iterAttOther =
				m_mapOtherAttributes.find(strAttName);

			if (iterAttKey != m_mapKeyAttributes.end()) {
				if (iterAttKey->second != att->as_string(0)) {
					return std::string("ERROR: Variable \"") + strName
						+ std::string("\" has inconsistent value of \"")
						+ strAttName + std::string("\" across files");
				}
			}
			if (iterAttOther != m_mapOtherAttributes.end()) {
				if (iterAttOther->second != att->as_string(0)) {
					return std::string("ERROR: Variable \"") + strName
						+ std::string("\" has inconsistent value of \"")
						+ strAttName + std::string("\" across files");
				}
			}
			if ((iterAttKey == m_mapKeyAttributes.end()) &&
			    (iterAttOther == m_mapOtherAttributes.end())
			) {
				return std::string("ERROR: Variable \"") + strName
					+ std::string("\" has inconsistent appearance of attribute \"")
					+ strAttName + std::string("\" across files");
			}
		}
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////
// FileListObjectConstructor
///////////////////////////////////////////////////////////////////////////////

std::string FileListObjectConstructor::Call(
	const ObjectRegistry & objreg,
	const std::vector<std::string> & vecCommandLine,
	const std::vector<ObjectType> & vecCommandLineType,
	Object ** ppReturn
) {
	FileListObject * pobjFileList = new FileListObject("");
	if (pobjFileList == NULL) {
		_EXCEPTIONT("Unable to initialize FileListObject");
	}

	// Constructor accepts 0 or 1 arguments
	if (vecCommandLine.size() > 1) {
		return std::string("ERROR: Invalid arguments to file_list()");
	}
	if (vecCommandLine.size() == 1) {
		if (vecCommandLineType[0] != ObjectType_String) {
			return std::string("ERROR: Invalid first argument [filename] "
				"in file_list() call");
		}

		// Initialize the FileList with the given search string
		std::string strError =
			pobjFileList->PopulateFromSearchString(
				vecCommandLine[0]);

		if (strError != "") {
			return strError;
		}
	}

	// Set the return value
	if (ppReturn != NULL) {
		(*ppReturn) = pobjFileList;
	} else {
		delete pobjFileList;
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////
// FileListObject
///////////////////////////////////////////////////////////////////////////////

const size_t FileListObject::InvalidFileIx = (-1);

///////////////////////////////////////////////////////////////////////////////

const size_t FileListObject::InvalidTimeIx = (-1);

///////////////////////////////////////////////////////////////////////////////

const long FileListObject::InconsistentDimensionSizes = (-1);

///////////////////////////////////////////////////////////////////////////////

FileListObject::~FileListObject() {
	for (int v = 0; v < m_vecVariableInfo.size(); v++) {
		delete m_vecVariableInfo[v];
	}
	for (int d = 0; d < m_vecDimensionInfo.size(); d++) {
		delete m_vecDimensionInfo[d];
	}
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::Call(
	const ObjectRegistry & objreg,
	const std::string & strFunctionName,
	const std::vector<std::string> & vecCommandLine,
	const std::vector<ObjectType> & vecCommandLineType,
	Object ** ppReturn
) {
	// Output information about the FileList to a CSV file
	if (strFunctionName == "output_csv") {
		if ((vecCommandLineType.size() != 1) ||
		    (vecCommandLineType[0] != ObjectType_String)
		) {
			return std::string("ERROR: Invalid parameters to function \"output_csv\"");
		}
		return OutputTimeVariableIndexCSV(vecCommandLine[0]);
	}

	// Create a copy of the FileList with modified filenames
	if (strFunctionName == "duplicate_for_writing") {
		if ((vecCommandLineType.size() != 1) ||
		    (vecCommandLineType[0] != ObjectType_String)
		) {
			return std::string("ERROR: Invalid parameters to function \"duplicate_for_writing\"");
		}

		FileListObject * pobjNewFileList = new FileListObject("");
		if (pobjNewFileList == NULL) {
			return std::string("ERROR: Unable to allocate new FileListObject");
		}

		pobjNewFileList->m_strBaseDir = vecCommandLine[0];
		if (vecCommandLine[0][vecCommandLine[0].length()-1] != '/') {
			pobjNewFileList->m_strBaseDir += "/";
		}

		// Create the directory if it doesn't exist
		DIR * pDir = opendir(pobjNewFileList->m_strBaseDir.c_str());
		if (pDir == NULL) {
			int iError =
				mkdir(
					pobjNewFileList->m_strBaseDir.c_str(),
					S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

			if (iError != 0) {
				return std::string("Unable to create directory \"")
					+ pobjNewFileList->m_strBaseDir + std::string("\"");
			}

		} else {
			closedir(pDir);
		}

		// Copy times
		pobjNewFileList->m_vecTimes = m_vecTimes;
		pobjNewFileList->m_mapTimeToIndex = m_mapTimeToIndex;
		pobjNewFileList->m_strTimeUnits = m_strTimeUnits;

/*
		// Add file extension
		for (size_t f = 0; f < pobjNewFileList->m_vecFilenames.size(); f++) {
			int nLength = pobjNewFileList->m_vecFilenames[f].length();
			int iExt = nLength-1;
			for (; iExt >= 0; iExt--) {
				if (pobjNewFileList->m_vecFilenames[f][iExt] == '.') {
					break;
				}
			}
			if (iExt == (-1)) {
				pobjNewFileList->m_vecFilenames[f] =
					m_vecFilenames[f]
					+ vecCommandLine[0];
			} else {
				pobjNewFileList->m_vecFilenames[f] =
					m_vecFilenames[f].substr(0,iExt)
					+ vecCommandLine[0]
					+ m_vecFilenames[f].substr(iExt);
			}
		}
*/
		if (ppReturn != NULL) {
			(*ppReturn) = pobjNewFileList;
		} else {
			delete pobjNewFileList;
		}
		return std::string("");
	}

	// Append files to the file_list
	if (strFunctionName == "append") {
		if ((vecCommandLineType.size() != 1) ||
		    (vecCommandLineType[0] != ObjectType_String)
		) {
			return std::string("ERROR: Invalid parameters to function \"add_file\"");
		}

		if (IsLocked()) {
			return std::string("ERROR: Cannot append files to a locked file_list");
		}

		return PopulateFromSearchString(m_strBaseDir + vecCommandLine[0]);
	}

	return
		Object::Call(
			objreg,
			strFunctionName,
			vecCommandLine,
			vecCommandLineType,
			ppReturn);
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::PopulateFromSearchString(
	const std::string & strSearchString
) {
	// Check if already initialized
	if (m_vecFilenames.size() != 0) {
		_EXCEPTIONT("FileListObject has already been initialized");
	}

	// File the directory in the search string
	std::string strFileSearchString;
	for (int i = strSearchString.length(); i >= 0; i--) {
		if (strSearchString[i] == '/') {
			m_strBaseDir = strSearchString.substr(0,i+1);
			strFileSearchString =
				strSearchString.substr(i+1, std::string::npos);
			break;
		}
	}
	if ((m_strBaseDir == "") && (strFileSearchString == "")) {
		strFileSearchString = strSearchString;
		m_strBaseDir = "./";
	}

	// Open the directory
	DIR * pDir = opendir(m_strBaseDir.c_str());
	if (pDir == NULL) {
		return std::string("Unable to open directory \"")
			+ m_strBaseDir + std::string("\"");
	}

	// Search all files in the directory for match to search string
	size_t iFileBegin = m_vecFilenames.size();
	struct dirent * pDirent;
	while ((pDirent = readdir(pDir)) != NULL) {
		std::string strFilename = pDirent->d_name;
		if (STLStringHelper::WildcardMatch(
				strFileSearchString.c_str(),
				strFilename.c_str())
		) {
			// File found, insert into list of filenames
			m_vecFilenames.push_back(strFilename);
		}
	}
	closedir(pDir);

	// Index the variable data
	return IndexVariableData(iFileBegin, m_vecFilenames.size());
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::CreateFilesFromTemplate(
	const std::string & strFilenameTemplate,
	const GridObject * pobjGrid,
	int nTimesPerFile
) {
	if (pobjGrid == NULL) {
		_EXCEPTION();
	}

	// Search for time specifier in template
	int iTemplatePos = (-1);
	for (int i = 0; i < strFilenameTemplate.length()-1; i++) {
		if ((strFilenameTemplate[i] == '%') &&
			(strFilenameTemplate[i+1] == 'T')
		) {
			if (iTemplatePos == (-1)) {
				iTemplatePos = i;
			} else {
				return std::string("ERROR: Time specifier appears more than once in filename template");
			}
		}
	}
	if (iTemplatePos == (-1)) {
		return std::string("ERROR: Time specifier missing from filename template");
	}

	// Loop through all times
	for (int t = 0; t < m_vecTimes.size(); t += nTimesPerFile) {
		std::string strFilename =
			strFilenameTemplate.substr(0,iTemplatePos)
			+ m_vecTimes[t].ToShortString()
			+ strFilenameTemplate.substr(iTemplatePos+2, std::string::npos);

		// Create the file from the Grid
		std::string strError =
			CreateFileNoTime(
				strFilename,
				pobjGrid);

		if (strError != "") {
			return strError;
		}

		// Add record dimension
		std::string strFullFilename = m_strBaseDir + strFilename;
		NcFile ncFile(strFullFilename.c_str(), NcFile::Write);
		if (!ncFile.is_valid()) {
			return std::string("ERROR: Unable to open \"")
				+ strFilename + std::string("\" for writing");
		}

		if (m_strRecordDimName == "") {
			return std::string("ERROR: NetCDF file template has no record dimension");
		}

		NcDim * dimRecord = ncFile.add_dim(m_strRecordDimName.c_str(), 0);
		if (dimRecord == NULL) {
			return std::string("ERROR: Unable to create record dimension \"")
				+ m_strRecordDimName + std::string("\" in file ")
				+ strFilename;
		}

		NcVar * varRecord = ncFile.add_var(m_strRecordDimName.c_str(), ncDouble, dimRecord);
		if (varRecord == NULL) {
			return std::string("ERROR: Unable to create record variable \"")
				+ m_strRecordDimName + std::string("\" in file ")
				+ strFilename;
		}

		NcBool fAttUnits = varRecord->add_att("units", m_strTimeUnits.c_str());
		if (!fAttUnits) {
			return std::string("ERROR: Unable to add attributes to variable \"")
				+ m_strRecordDimName + std::string("\" in file ")
				+ strFilename;
		}

		// Output times
		int nTimes = nTimesPerFile;
		if (t + nTimes >= m_vecTimes.size()) {
			nTimes = m_vecTimes.size() - t;
		}

		std::vector<double> dTimes;
		dTimes.resize(nTimes);
		for (int s = 0; s < nTimes; s++) {
			dTimes[s] = m_vecTimes[t+s].GetCFCompliantUnitsOffsetDouble(m_strTimeUnits);
		}
		varRecord->put(&(dTimes[0]), nTimes);

		// Populate m_mapOutputTimeFile
		for (int s = 0; s < nTimes; s++) {
			m_mapOutputTimeFile.insert(
				std::pair<size_t, LocalFileTimePair>(
					t+s, LocalFileTimePair(t/nTimesPerFile, s)));
		}
	}
	
	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::CreateFileNoTime(
	const std::string & strFilename,
	const GridObject * pobjGrid
) {
	_EXCEPTION();
/*
	if (pobjGrid == NULL) {
		_EXCEPTION();
	}

	// Check if file already exists
	for (size_t f = 0; f < m_vecFilenames.size(); f++) {
		if (m_vecFilenames[f] == strFilename) {
			return std::string("ERROR: File \"") + strFilename
				+ std::string("\" already exists in file_list");
		}
	}

	// Add the file to the registry
	size_t sNewFileIx = m_vecFilenames.size();
	m_vecFilenames.push_back(strFilename);

	std::string strFullFilename = m_strBaseDir + strFilename;

	NcFile ncFile(strFullFilename.c_str(), NcFile::Replace);
	if (!ncFile.is_valid()) {
		return std::string("ERROR: Unable to open \"")
			+ strFullFilename + std::string("\" for writing");
	}

	// Add grid dimensions to file
	const Mesh & mesh = pobjGrid->GetMesh();
	for (size_t i = 0; i < mesh.vecDimNames.size(); i++) {
		NcDim * dim =
			ncFile.add_dim(
				mesh.vecDimNames[i].c_str(),
				mesh.vecDimSizes[i]);

		if (dim == NULL) {
			_EXCEPTION1("Unable to create dimension \"%s\" in file",
				mesh.vecDimNames[i].c_str());
		}

		if (mesh.vecDimValues.size() > i) {
			if (mesh.vecDimValues[i].size() != mesh.vecDimSizes[i]) {
				_EXCEPTIONT("Mesh DimValues / DimSizes mismatch");
			}

			NcVar * var =
				ncFile.add_var(
					mesh.vecDimNames[i].c_str(),
					ncDouble,
					dim);

			if (var == NULL) {
				_EXCEPTION1("Unable to create variable \"%s\" in file",
					mesh.vecDimNames[i].c_str());
			}

			var->set_cur((long)0);
			var->put(&(mesh.vecDimValues[i][0]), mesh.vecDimSizes[i]);
		}
	}
*/
	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::SetReduceTarget(
	const std::string & strTargetFilename
) {

	// Check if file exists
	for (size_t f = 0; f < m_vecFilenames.size(); f++) {
		if (m_vecFilenames[f] == strTargetFilename) {
			m_sReduceTargetIx = f;
			return std::string("");
		}
	}

	return std::string("ERROR: Reduce target not in file_list");
}

///////////////////////////////////////////////////////////////////////////////

bool FileListObject::IsCompatible(
	const FileListObject * pobjFileList
) {
	if (m_vecTimes.size() != pobjFileList->m_vecTimes.size()) {
		return false;
	}
	for (size_t t = 0; t < m_vecTimes.size(); t++) {
		if (m_vecTimes[t] != pobjFileList->m_vecTimes[t]) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

void FileListObject::GetOnRankTimeIndices(
	std::vector<size_t> & vecTimeIndices,
	size_t sTimeStride
) {
	if ((sTimeStride == 0) || (sTimeStride > 1000)) {
		_EXCEPTIONT("timestride out of range");
	}

#if defined(HYPERION_MPIOMP)
	size_t sTimeCount = m_vecTimes.size() / sTimeStride;

	int nCommRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &nCommRank);

	int nCommSize;
	MPI_Comm_size(MPI_COMM_WORLD, &nCommSize);

	int nMaxTimesPerRank = static_cast<int>(sTimeCount) / nCommSize;
	if (sTimeCount % nCommSize != 0) {
		nMaxTimesPerRank++;
	}

	int iBegin = nMaxTimesPerRank * nCommRank;
	int iEnd = nMaxTimesPerRank * (nCommRank + 1);
	if (iEnd > sTimeCount) {
		iEnd = sTimeCount;
	}

	for (size_t i = iBegin; i < iEnd; i++) {
		vecTimeIndices.push_back(i * sTimeStride);
	}
#else
	for (size_t i = 0; i < m_vecTimes.size(); i += sTimeStride) {
		vecTimeIndices.push_back(i);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::LoadData_float(
	const std::string & strVariableName,
	const std::vector<long> & vecAuxIndices,
	DataArray1D<float> & data
) {
	// Find the VariableInfo structure for this Variable
	size_t iVarInfo = 0;
	for (; iVarInfo < m_vecVariableInfo.size(); iVarInfo++) {
		if (strVariableName == m_vecVariableInfo[iVarInfo]->m_strName) {
			break;
		}
	}
	if (iVarInfo == m_vecVariableInfo.size()) {
		_EXCEPTION1("Variable \"%s\" not found in file_list index",
			strVariableName.c_str());
	}

	// Find the local file/time pair associated with this global time index
	VariableInfo & varinfo = *(m_vecVariableInfo[iVarInfo]);

	// Get the time index
	if ((varinfo.m_iTimeDimIx != (-1)) &&
		(varinfo.m_iTimeDimIx >= vecAuxIndices.size())
	) {
		_EXCEPTIONT("time index exceeds auxiliary index size");
	}

	// Extract the global time index
	size_t sTime = (-1);
	if (varinfo.m_iTimeDimIx != (-1)) {
		sTime = vecAuxIndices[varinfo.m_iTimeDimIx];
	}

	// Find local file/time index
	VariableTimeFileMap::const_iterator iter =
		varinfo.m_mapTimeFile.find(sTime);

	if (iter == varinfo.m_mapTimeFile.end()) {
		_EXCEPTION2("sTime (%s) (%lu) not found", strVariableName.c_str(), sTime);
	}

	size_t sFile = iter->second.first;
	int iTime = iter->second.second;

	{
		std::string strLoading =
			std::string("READ ") + Name() + std::string("[") + strVariableName + std::string("]");

		for (size_t d = 0; d < vecAuxIndices.size(); d++) {
			if (d == varinfo.m_iTimeDimIx) {
				strLoading +=
					std::string(" [")
					+ m_vecTimes[sTime].ToString()
					+ std::string("]");
			} else {
				strLoading +=
					std::string(" [")
					+ varinfo.m_vecAuxDimNames[d]
					+ std::string(": ")
					+ std::to_string(vecAuxIndices[d])
					+ std::string("]");
			}
		}
		Announce(strLoading.c_str());
	}

	// Open the correct NetCDF file
	std::string strFullFilename = m_strBaseDir + m_vecFilenames[sFile];
	NcFile ncfile(strFullFilename.c_str());
	if (!ncfile.is_valid()) {
		_EXCEPTION1("Cannot open file \"%s\"", strFullFilename.c_str());
	}

	// Get the correct variable from the file
	NcVar * var = ncfile.get_var(strVariableName.c_str());
	const long nDims = var->num_dims() - m_vecGridDimNames.size();

	if (var == NULL) {
		_EXCEPTION1("Variable \"%s\" no longer found in file",
			strVariableName.c_str());
	}
	if ((var->type() != ncFloat) && (var->type() != ncDouble)) {
		return std::string("Variable \"")
			+ strVariableName
			+ std::string("\" is not of type float or double");
	}
	if (nDims != vecAuxIndices.size()) {
		_EXCEPTION2("Auxiliary index array size mismatch (%li / %lu)",
			nDims, vecAuxIndices.size());
	}

	// Set the data position and size
	long lTotalSize = 1;
	std::vector<long> vecPos = vecAuxIndices;
	std::vector<long> vecSize = vecAuxIndices;

	for (size_t d = 0; d < vecAuxIndices.size(); d++) {
		if (d == varinfo.m_iTimeDimIx) {
			vecPos[d] = iTime;
		}
		vecSize[d] = 1;
	}
	for (size_t d = 0; d < m_vecGridDimNames.size(); d++) {
		NcDim * dimGrid = var->get_dim(vecPos.size());
		vecPos.push_back(0);
		vecSize.push_back(dimGrid->size());
		lTotalSize *= dimGrid->size();
	}

	if (data.GetRows() != lTotalSize) {
		//for (int d = 0; d < vecAuxIndices.size(); d++) {
		//	printf("%s %i\n", varinfo.m_vecDimNames[d].c_str(), vecAuxIndices[d]);
		//}
		//for (int d = 0; d < nDims; d++) {
		//	printf("%i %i\n", vecPos[d], vecSize[d]);
		//}
		_EXCEPTION2("Data size mismatch (%i/%lu)", data.GetRows(), lTotalSize);
	}

	// Set the position
	var->set_cur(&(vecPos[0]));

	// Load the data
/*
	std::cout << varinfo.m_strName << std::endl;
		for (int d = 0; d < vecAuxIndices.size(); d++) {
			printf("%s %i\n", varinfo.m_vecDimNames[d].c_str(), vecAuxIndices[d]);
		}
		for (int d = 0; d < nDims; d++) {
			printf("%i %i\n", vecPos[d], vecSize[d]);
		}
*/
	if (var->type() == ncDouble) {
		DataArray1D<double> data_dbl(data.GetRows());
		var->get(&(data_dbl[0]), &(vecSize[0]));
		for (int i = 0; i < data.GetRows(); i++) {
			data[i] = static_cast<float>(data_dbl[i]);
		}
	} else {
		var->get(&(data[0]), &(vecSize[0]));
	}

	NcError err;
	if (err.get_err() != NC_NOERR) {
		_EXCEPTION1("NetCDF Fatal Error (%i)", err.get_err());
	}

	// Cleanup
	ncfile.close();

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::WriteData_float(
	const std::string & strVariableName,
	const std::vector<long> & vecAuxIndices,
	const DataArray1D<float> & data
) {
	// Find the VariableInfo structure for this Variable
	size_t iVarInfo = 0;
	for (; iVarInfo < m_vecVariableInfo.size(); iVarInfo++) {
		if (strVariableName == m_vecVariableInfo[iVarInfo]->m_strName) {
			break;
		}
	}

	// File index for write
	size_t sFile = (-1);
	int iLocalTime = (-1);

	// Not found
	if (iVarInfo == m_vecVariableInfo.size()) {
		_EXCEPTION();
	}

	// Get file index from varinfo
	VariableInfo & varinfo = *(m_vecVariableInfo[iVarInfo]);

	// Check auxiliary indices
	if (vecAuxIndices.size() != varinfo.m_vecAuxDimSizes.size()) {
		_EXCEPTIONT("Invalid auxiliary index");
	}

	// Extract the time index
	size_t sTime = (-1);
	if (varinfo.m_iTimeDimIx != (-1)) {
		sTime = vecAuxIndices[varinfo.m_iTimeDimIx];
	}

	// Write message
	{
		std::string strWriting =
			std::string("WRITE ") + Name() + std::string("[") + strVariableName + std::string("]");

		for (size_t d = 0; d < vecAuxIndices.size(); d++) {
			if (d == varinfo.m_iTimeDimIx) {
				strWriting +=
					std::string(" [")
					+ m_vecTimes[sTime].ToString()
					+ std::string("]");
			} else {
				strWriting +=
					std::string(" [")
					+ varinfo.m_vecAuxDimNames[d]
					+ std::string(": ")
					+ std::to_string(vecAuxIndices[d])
					+ std::string("]");
			}
		}
		Announce(strWriting.c_str());
	}

	// Check consistency with indexing procedure
	if ((sTime == (-1)) && (varinfo.m_iTimeDimIx != (-1))) {
		_EXCEPTIONT("Attempting to write single time index "
			"for multi-time-index variable");
	}
	if ((sTime != (-1)) && (varinfo.m_iTimeDimIx == (-1))) {
		_EXCEPTIONT("Attempting to write single time indexed "
			"variable as multi-indexed variable");
	}

	VariableTimeFileMap::const_iterator iterTimeFile =
		varinfo.m_mapTimeFile.find(sTime);

	if (iterTimeFile == varinfo.m_mapTimeFile.end()) {
		if (sTime == InvalidTimeIx) {
			sFile = m_sReduceTargetIx;
			iLocalTime = (-1);

		} else {
			std::map<size_t, LocalFileTimePair>::const_iterator iterFileIx =
				m_mapOutputTimeFile.find(sTime);

			if (iterFileIx == m_mapOutputTimeFile.end()) {
				_EXCEPTIONT("Unable to determine output file");
			}

			sFile = iterFileIx->second.first;
			iLocalTime = iterFileIx->second.second;

			varinfo.m_mapTimeFile.insert(
				VariableTimeFileMap::value_type(
					sTime,
					LocalFileTimePair(sFile, iLocalTime)));
		}

	} else {
		sFile = iterTimeFile->second.first;
		iLocalTime = iterTimeFile->second.second;
	}

	if (sFile == (-1)) {
		_EXCEPTIONT("Logic error");
	}

	// Set the data position and size
	long lTotalSize = 1;
	std::vector<long> vecPos = vecAuxIndices;
	std::vector<long> vecSize = vecAuxIndices;

	for (size_t d = 0; d < vecAuxIndices.size(); d++) {
		if (d == varinfo.m_iTimeDimIx) {
			vecPos[d] = iLocalTime;
		}
		vecSize[d] = 1;
	}
	for (size_t d = 0; d < m_vecGridDimNames.size(); d++) {
		DimensionInfoMap::const_iterator iter =
			m_mapDimensionInfo.find(m_vecGridDimNames[d]);

		if (iter == m_mapDimensionInfo.end()) {
			_EXCEPTIONT("Dimension not found in map");
		}

		vecPos.push_back(0);
		vecSize.push_back(iter->second.m_lSize);
		lTotalSize *= iter->second.m_lSize;
	}
	//for (size_t d = 0; d < m_vecGridDimNames.size(); d++) {
	//	printf("DIM %s %i/%i\n", varinfo.m_vecDimNames[d].c_str(), vecPos[d], vecSize[d]);
	//}

	if (data.GetRows() != lTotalSize) {
		_EXCEPTION2("Data size mismatch (%i/%lu)", data.GetRows(), lTotalSize);
	}

	// Write data
	std::string strFullFilename = m_strBaseDir + m_vecFilenames[sFile];
	NcFile ncout(strFullFilename.c_str(), NcFile::Write);
	if (!ncout.is_valid()) {
		_EXCEPTION1("Unable to open output file \"%s\"", strFullFilename.c_str());
	}

	// Get dimensions
	std::vector<NcDim *> vecDims;
	vecDims.resize(varinfo.m_vecDimNames.size());
	for (int d = 0; d < vecDims.size(); d++) {
		long lDimSize = GetDimensionSize(varinfo.m_vecDimNames[d]);
		vecDims[d] = ncout.get_dim(varinfo.m_vecDimNames[d].c_str());
		if (vecDims[d] != NULL) {
			if (d != varinfo.m_iTimeDimIx) {
				if (vecDims[d]->size() != lDimSize) {
					_EXCEPTION3("Dimension %s mismatch (%lu / %li)",
						varinfo.m_vecDimNames[d].c_str(),
						vecDims[d]->size(), lDimSize);
				}
			}

		} else {

			// Create a new dimension in output file
			vecDims[d] =
				ncout.add_dim(
					varinfo.m_vecDimNames[d].c_str(),
					lDimSize);

			if (vecDims[d] == NULL) {
				_EXCEPTION1("Cannot add dimension %s",
					varinfo.m_vecDimNames[d].c_str());
			}

			DimensionInfoMap::const_iterator iterDimInfo =
				m_mapDimensionInfo.find(varinfo.m_vecDimNames[d]);

			// Create a dimension variable in output file
			if (iterDimInfo->second.m_dValuesDouble.size() != 0) {
				NcVar * varDim =
					ncout.add_var(
						varinfo.m_vecDimNames[d].c_str(),
						ncDouble,
						vecDims[d]);
				if (varDim == NULL) {
					_EXCEPTION1("Cannot add variable %s",
						varinfo.m_vecDimNames[d].c_str());
				}
				varDim->set_cur((long)0);
				varDim->put(
					&(iterDimInfo->second.m_dValuesDouble[0]),
					iterDimInfo->second.m_dValuesDouble.size());

				if (iterDimInfo->second.m_strUnits != "") {
					varDim->add_att("units",
						iterDimInfo->second.m_strUnits.c_str());
				}
			}
		}
	}

	// Create variable
	NcVar * var = ncout.get_var(strVariableName.c_str());
	if (var == NULL) {
		var = ncout.add_var(
			strVariableName.c_str(),
			ncFloat,
			vecDims.size(),
			(const NcDim **)(&(vecDims[0])));

		if (var == NULL) {
			_EXCEPTION1("Unable to create variable \"%s\"",
				strVariableName.c_str());
		}

		var->add_att("units", varinfo.m_strUnits.c_str());
	}

	// Set current position
	var->set_cur(&(vecPos[0]));

	// Write data
	var->put(&(data[0]), &(vecSize[0]));

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::AddVariableFromTemplate(
	const FileListObject * pobjSourceFileList,
	const Variable * pvar,
	VariableInfo ** ppvarinfo
) {
	_EXCEPTION();
/*
	// Check arguments
	if (ppvarinfo == NULL) {
		_EXCEPTIONT("Invalid value for \"ppvarinfo\"");
	}

	// Check record dimension name equivalence
	if (GetRecordDimName() != pobjSourceFileList->GetRecordDimName()) {
		_EXCEPTIONT("Record dim name mismatch");
	}

	// Check if variable already exists
	for (int v = 0; v < m_vecVariableInfo.size(); v++) {
		if (m_vecVariableInfo[v]->m_strName == pvar->Name()) {
			return std::string("ERROR: Variable already exists in file_list");
		}
	}

	// VariableInfo
	VariableInfo * pvarinfo = new VariableInfo(pvar->Name());
	if (pvarinfo == NULL) {
		_EXCEPTIONT("Unable to allocate VariableInfo");
	}

	pvarinfo->m_strUnits = pvar->Units();

	// Add dimensions from variable to output FileList
	bool fHasRecord = false;
	bool fHasVertical = false;
	const std::vector<std::string> & vecDimNames = pvar->AuxDimNames();
	for (int d = 0; d < vecDimNames.size(); d++) {

		DimensionInfoMap::const_iterator iterDimInfoSource =
			pobjSourceFileList->m_mapDimensionInfo.find(vecDimNames[d]);

		if (iterDimInfoSource == pobjSourceFileList->m_mapDimensionInfo.end()) {
			_EXCEPTIONT("Logic error");
		}

		const DimensionInfo & diminfoSource = iterDimInfoSource->second;

		if (diminfoSource.m_eType == DimensionInfo::Type_Unknown) {
			_EXCEPTIONT("Unknown dimension type");
		
		} else if (diminfoSource.m_eType == DimensionInfo::Type_Grid) {
			return std::string("ERROR: Grid dimensions appear in auxiliary dimension list for variable \"")
				+ vecDimNames[d] + std::string("\"");

		} else if (diminfoSource.m_eType == DimensionInfo::Type_Vertical) {
			if (fHasVertical) {
				return std::string("Variable \"") + vecDimNames[d]
					+ std::string("\" appears to have multiple vertical dimensions");
			}
			if ((vecDimNames[d] != "lev") &&
				(vecDimNames[d] != "pres") &&
				(vecDimNames[d] != "z") &&
				(vecDimNames[d] != "plev")
			) {
				return std::string("Invalid vertical dimension name \"")
					+ vecDimNames[d] + std::string("\"");
			}

			fHasVertical = true;

			pvarinfo->m_iVerticalDimIx = d;
			pvarinfo->m_nVerticalDimOrder = diminfoSource.m_nOrder;

		} else if (diminfoSource.m_eType == DimensionInfo::Type_Record) {
			if (fHasRecord) {
				return std::string("Variable \"") + vecDimNames[d]
					+ std::string("\" appears to have multiple record dimensions");
			}

			fHasRecord = true;

			pvarinfo->m_iTimeDimIx = d;

		} else if (diminfoSource.m_eType == DimensionInfo::Type_Auxiliary) {

		} else {
			_EXCEPTIONT("Invalid DimensionInfo::m_eType");
		}

		pvarinfo->m_vecDimNames.push_back(vecDimNames[d]);
		pvarinfo->m_vecDimSizes.push_back(diminfoSource.m_lSize);
		pvarinfo->m_vecAuxDimNames.push_back(vecDimNames[d]);
		pvarinfo->m_vecAuxDimSizes.push_back(diminfoSource.m_lSize);

		// Check for existence of dimension in dimension structure
		DimensionInfoMap::const_iterator iterDimInfo =
			m_mapDimensionInfo.find(vecDimNames[d]);

		if (iterDimInfo != m_mapDimensionInfo.end()) {
			if (iterDimInfo->second != diminfoSource) {
				return std::string("ERROR: Inconsistent dimension \"")
					+ vecDimNames[d] + std::string(" already exists in output file_list");
			}
			continue;
		}

		// If it doesn't exist, insert it into the structure
		m_mapDimensionInfo.insert(
			DimensionInfoMap::value_type(
				vecDimNames[d],
				iterDimInfoSource->second));
	}

	if (!fHasVertical) {
		pvarinfo->m_iVerticalDimIx = (-1);
	}
	if (!fHasRecord) {
		pvarinfo->m_iTimeDimIx = (-1);
	}

	// Add grid dimensions
	for (int d = 0; d < m_vecGridDimNames.size(); d++) {
		pvarinfo->m_vecDimNames.push_back(m_vecGridDimNames[d]);

		DimensionInfoMap::const_iterator iterDimInfo =
			m_mapDimensionInfo.find(m_vecGridDimNames[d]);
		if (iterDimInfo == m_mapDimensionInfo.end()) {
			_EXCEPTIONT("Grid dimensions missing from DimensionInfo map");
		}
		pvarinfo->m_vecDimSizes.push_back(iterDimInfo->second.m_lSize);
	}

	// Add this VariableInfo to the vector of VariableInfos
	m_vecVariableInfo.push_back(pvarinfo);

	(*ppvarinfo) = pvarinfo;

	// Sanity checks
	if (pvarinfo->m_vecDimNames.size() != pvarinfo->m_vecDimSizes.size()) {
		_EXCEPTIONT("Logic error");
	}
	if (pvarinfo->m_vecAuxDimNames.size() != pvarinfo->m_vecAuxDimSizes.size()) {
		_EXCEPTIONT("Logic error");
	}
	if (pvarinfo->m_vecAuxDimSizes.size() > pvarinfo->m_vecDimSizes.size()) {
		_EXCEPTIONT("Logic error");
	}
*/
	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::AddVariableFromTemplateWithNewVerticalDim(
	const FileListObject * pobjSourceFileList,
	const Variable * pvar,
	const std::string & strVerticalDimName,
	VariableInfo ** ppvarinfo
) {
	_EXCEPTION();
/*
	// Check arguments
	if (ppvarinfo == NULL) {
		_EXCEPTIONT("Invalid value for \"ppvarinfo\"");
	}

	if ((strVerticalDimName != "lev") &&
		(strVerticalDimName != "pres") &&
		(strVerticalDimName != "z") &&
		(strVerticalDimName != "plev")
	) {
		_EXCEPTIONT("Invalid name for vertical dimension");
	}

	// Get the FileList object from the other RecapConfig
	if (GetRecordDimName() != pobjSourceFileList->GetRecordDimName()) {
		_EXCEPTIONT("Record dim name mismatch");
	}

	// Check if variable already exists
	for (int v = 0; v < m_vecVariableInfo.size(); v++) {
		if (m_vecVariableInfo[v]->m_strName == pvar->Name()) {
			return std::string("ERROR: Variable already exists in file_list");
		}
	}

	// Get the vertical dimension info
	DimensionInfoMap::const_iterator iterDimInfo =
		m_mapDimensionInfo.find(strVerticalDimName);

	if (iterDimInfo == m_mapDimensionInfo.end()) {
		return std::string("Vertical dimension \"") + strVerticalDimName
			+ std::string("\" not found in ") + m_strName;
	}

	long lVerticalDimSize = iterDimInfo->second.m_lSize;
	int nVerticalDimOrder = iterDimInfo->second.m_nOrder;

	if (lVerticalDimSize < 1) {
		_EXCEPTIONT("Invalid size for vertical dimension");
	}
	if (abs(nVerticalDimOrder) != 1) {
		_EXCEPTIONT("Invalid order for vertical dimension");
	}

	// VariableInfo
	VariableInfo * pvarinfo = new VariableInfo(pvar->Name());
	if (pvarinfo == NULL) {
		_EXCEPTIONT("Unable to allocate VariableInfo");
	}

	pvarinfo->m_strUnits = pvar->Units();

	// Add dimensions from variable to output FileList
	bool fHasRecord = false;
	bool fHasVertical = false;
	const std::vector<std::string> & vecDimNames = pvar->AuxDimNames();
	for (int d = 0; d < vecDimNames.size(); d++) {

		DimensionInfoMap::const_iterator iterDimInfoSource =
			pobjSourceFileList->m_mapDimensionInfo.find(vecDimNames[d]);

		if (iterDimInfoSource == pobjSourceFileList->m_mapDimensionInfo.end()) {
			_EXCEPTIONT("Logic error");
		}

		const DimensionInfo & diminfoSource = iterDimInfoSource->second;

		if (diminfoSource.m_eType == DimensionInfo::Type_Unknown) {
			_EXCEPTIONT("Unknown dimension type");
		
		} else if (diminfoSource.m_eType == DimensionInfo::Type_Grid) {
			return std::string("ERROR: Grid dimensions appear in auxiliary dimension list for variable \"")
				+ vecDimNames[d] + std::string("\"");

		} else if (diminfoSource.m_eType == DimensionInfo::Type_Vertical) {
			if (fHasVertical) {
				return std::string("ERROR: Variable \"") + vecDimNames[d]
					+ std::string("\" appears to have multiple vertical dimensions");
			}
			if ((vecDimNames[d] != "lev") &&
				(vecDimNames[d] != "pres") &&
				(vecDimNames[d] != "z") &&
				(vecDimNames[d] != "plev")
			) {
				return std::string("Invalid vertical dimension name \"")
					+ vecDimNames[d] + std::string("\"");
			}

			fHasVertical = true;

			pvarinfo->m_iVerticalDimIx = d;
			pvarinfo->m_nVerticalDimOrder = nVerticalDimOrder;
			pvarinfo->m_vecDimNames.push_back(strVerticalDimName);
			pvarinfo->m_vecDimSizes.push_back(lVerticalDimSize);
			pvarinfo->m_vecAuxDimNames.push_back(strVerticalDimName);
			pvarinfo->m_vecAuxDimSizes.push_back(lVerticalDimSize);
			continue;

		} else if (diminfoSource.m_eType == DimensionInfo::Type_Record) {
			if (fHasRecord) {
				return std::string("ERROR: Variable \"") + vecDimNames[d]
					+ std::string("\" appears to have multiple record dimensions");
			}

			fHasRecord = true;

			pvarinfo->m_iTimeDimIx = d;

		} else if (diminfoSource.m_eType == DimensionInfo::Type_Auxiliary) {

		} else {
			_EXCEPTIONT("Invalid DimensionInfo::m_eType");
		}

		pvarinfo->m_vecDimNames.push_back(vecDimNames[d]);
		pvarinfo->m_vecDimSizes.push_back(diminfoSource.m_lSize);
		pvarinfo->m_vecAuxDimNames.push_back(vecDimNames[d]);
		pvarinfo->m_vecAuxDimSizes.push_back(diminfoSource.m_lSize);

		// Check for existence of dimension in dimension structure
		DimensionInfoMap::const_iterator iterDimInfo =
			m_mapDimensionInfo.find(vecDimNames[d]);

		if (iterDimInfo != m_mapDimensionInfo.end()) {
			if (iterDimInfo->second != diminfoSource) {
				return std::string("ERROR: Inconsistent dimension \"")
					+ vecDimNames[d] + std::string(" already exists in output file_list");
			}
			continue;
		}

		// If it doesn't exist, insert it into the structure
		m_mapDimensionInfo.insert(
			DimensionInfoMap::value_type(
				vecDimNames[d],
				iterDimInfoSource->second));
	}

	if (!fHasVertical) {
		return std::string("ERROR: Variable \"") + Name()
			+ std::string("[") + pvarinfo->m_strName
			+ std::string("]\" has no vertical dimension");
	}
	if (!fHasRecord) {
		pvarinfo->m_iTimeDimIx = (-1);
	}

	// Add grid dimensions
	for (int d = 0; d < m_vecGridDimNames.size(); d++) {
		pvarinfo->m_vecDimNames.push_back(m_vecGridDimNames[d]);

		DimensionInfoMap::const_iterator iterDimInfo =
			m_mapDimensionInfo.find(m_vecGridDimNames[d]);
		if (iterDimInfo == m_mapDimensionInfo.end()) {
			_EXCEPTIONT("Grid dimensions missing from DimensionInfo map");
		}
		pvarinfo->m_vecDimSizes.push_back(iterDimInfo->second.m_lSize);
	}

	// Add this VariableInfo to the vector of VariableInfos
	m_vecVariableInfo.push_back(pvarinfo);

	(*ppvarinfo) = pvarinfo;

	if (pvarinfo->m_vecDimNames.size() != pvarinfo->m_vecDimSizes.size()) {
		_EXCEPTIONT("Logic error");
	}
	if (pvarinfo->m_vecAuxDimNames.size() != pvarinfo->m_vecAuxDimSizes.size()) {
		_EXCEPTIONT("Logic error");
	}
	if (pvarinfo->m_vecAuxDimSizes.size() > pvarinfo->m_vecDimSizes.size()) {
		_EXCEPTIONT("Logic error");
	}
*/
	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::AddDimension(
	const std::string & strDimName,
	long lDimSize,
	DimensionInfo::Type eDimType
) {
	if ((eDimType != DimensionInfo::Type_Auxiliary) &&
		(eDimType != DimensionInfo::Type_Grid) &&
		(eDimType != DimensionInfo::Type_Record) &&
		(eDimType != DimensionInfo::Type_Vertical)
	) {
		_EXCEPTIONT("Invalid eDimType");
	}

	DimensionInfo diminfo;
	diminfo.m_strName = strDimName;
	diminfo.m_lSize = lDimSize;
	diminfo.m_eType = eDimType;

	// Find if dimension exists in dimension map
	DimensionInfoMap::iterator iter =
		m_mapDimensionInfo.find(strDimName);

	if (iter != m_mapDimensionInfo.end()) {
		if (iter->second.m_eType == DimensionInfo::Type_Auxiliary) {
			iter->second.m_eType = diminfo.m_eType;
		}
		if (diminfo != iter->second) {
			std::string strDimInfo = diminfo.ToString();
			std::string strIter = iter->second.ToString();
			_EXCEPTION2("Dimension info mismatch:\n%s\n%s",
				strDimInfo.c_str(), strIter.c_str());
		}

	} else {
		m_mapDimensionInfo.insert(
			DimensionInfoMap::value_type(strDimName, diminfo));
	}

	// Add a grid dimension
	if (eDimType == DimensionInfo::Type_Grid) {
		bool fFound = false;
		for (size_t d = 0; d < m_vecGridDimNames.size(); d++) {
			if (m_vecGridDimNames[d] == strDimName) {
				fFound = true;
				break;
			}
		}
		if (!fFound) {
			m_vecGridDimNames.push_back(strDimName);
			for (size_t v = 0; v < m_vecVariableInfo.size(); v++) {
				if (m_vecVariableInfo[v]->m_vecDimNames.size() !=
					m_vecVariableInfo[v]->m_vecDimSizes.size()
				) {
					_EXCEPTIONT("Logic error");
				}
				for (size_t d = 0; d < m_vecVariableInfo[v]->m_vecAuxDimNames.size(); d++) {
					if (m_vecVariableInfo[v]->m_vecAuxDimNames[d] == strDimName) {
						m_vecVariableInfo[v]->m_vecAuxDimNames.erase(
							m_vecVariableInfo[v]->m_vecAuxDimNames.begin() + d);
						m_vecVariableInfo[v]->m_vecAuxDimSizes.erase(
							m_vecVariableInfo[v]->m_vecAuxDimSizes.begin() + d);
						d--;
					}
				}
			}
		}
	}

	// Add a vertical dimension
	if (eDimType == DimensionInfo::Type_Vertical) {
		_EXCEPTION();
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::AddVerticalDimension(
	const std::string & strDimName,
	const std::vector<double> & dDimValues,
	const std::string & strDimUnits
) {
	DimensionInfo diminfo(strDimName);
	diminfo.m_eType = DimensionInfo::Type_Vertical;
	diminfo.m_lSize = dDimValues.size();
	diminfo.m_dValuesDouble = dDimValues;
	diminfo.m_strUnits = strDimUnits;

	// Determine order of target vertical dimension and verify monotonicity
	diminfo.m_nOrder = (+1);
	if (dDimValues.size() != 0) {
		if (dDimValues.size() > 1) {
			if (dDimValues[1] > dDimValues[0]) {
				diminfo.m_nOrder = (+1);
				for (size_t s = 0; s < dDimValues.size()-1; s++) {
					if (dDimValues[s+1] <= dDimValues[s]) {
						return std::string("ERROR: Vertical levels must be monotonic");
					}
				}
			} else if (dDimValues[1] < dDimValues[0]) {
				diminfo.m_nOrder = (-1);
				for (size_t s = 0; s < dDimValues.size()-1; s++) {
					if (dDimValues[s+1] >= dDimValues[s]) {
						return std::string("ERROR: Vertical levels must be monotonic");
					}
				}
			} else {
				return std::string("ERROR: Vertical levels must be monotonic");
			}
		}
	}

	// Check if already exists in set
	DimensionInfoMap::const_iterator iter =
		m_mapDimensionInfo.find(strDimName);

	if (iter != m_mapDimensionInfo.end()) {
		if (iter->second != diminfo) {
			return std::string("ERROR: Inconsistent dimension \"")
				+ strDimName + std::string(" already exists in output file_list");
		}

		return std::string("");
	}

	// Insert the dimension in the set
	m_mapDimensionInfo.insert(
		DimensionInfoMap::value_type(strDimName, diminfo));

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

long FileListObject::GetDimensionSize(
	const std::string & strDimName
) const {
	DimensionInfoMap::const_iterator iter =
		m_mapDimensionInfo.find(strDimName);

	if (iter != m_mapDimensionInfo.end()) {
		return iter->second.m_lSize;
	} else {
		return (-1);
	}
}

///////////////////////////////////////////////////////////////////////////////

void FileListObject::SortTimeArray() {

	if (m_vecTimes.size() != m_mapTimeToIndex.size()) {
		_EXCEPTIONT("vecTimes / mapTimeToIndex mismatch");
	}

	// Check if the array needs sorting, and map from old indices to new
	bool fSorted = true;
	std::map<size_t, size_t>  mapTimeIxToNewTimeIx;
	std::map<Time, size_t>::iterator iterTime = m_mapTimeToIndex.begin();
	for (size_t i = 0; iterTime != m_mapTimeToIndex.end(); iterTime++, i++) {
		if ((fSorted) && (i != iterTime->second)) {
			fSorted = false;
		}
		mapTimeIxToNewTimeIx.insert(
			std::pair<size_t, size_t>(iterTime->second, i));
	}

	if (fSorted) {
		return;
	}

	// Sort m_vecTimes and m_mapTimeToIndex
	iterTime = m_mapTimeToIndex.begin();
	for (size_t i = 0; iterTime != m_mapTimeToIndex.end(); iterTime++, i++) {
		m_vecTimes[i] = iterTime->first;
		iterTime->second = i;
	}

	// Rebuild VariableInfo VariableTimeFileMap with new time indices
	for (size_t i = 0; i < m_vecVariableInfo.size(); i++) {	
		VariableTimeFileMap mapTimeFileBak = m_vecVariableInfo[i]->m_mapTimeFile;
		m_vecVariableInfo[i]->m_mapTimeFile.clear();

		VariableTimeFileMap::const_iterator iterFileMap = mapTimeFileBak.begin();
		for (; iterFileMap != mapTimeFileBak.end(); iterFileMap++) {
			if (iterFileMap->first == InvalidTimeIx) {
				m_vecVariableInfo[i]->m_mapTimeFile.insert(
					VariableTimeFileMap::value_type(
						InvalidTimeIx,
						iterFileMap->second));
			} else {
				m_vecVariableInfo[i]->m_mapTimeFile.insert(
					VariableTimeFileMap::value_type(
						mapTimeIxToNewTimeIx[iterFileMap->first],
						iterFileMap->second));
			}
		}
	}
/*
	// Get the VariableRegistry
	if (m_pobjRecapConfig != NULL) {
		VariableRegistry & varreg =
			m_pobjRecapConfig->GetVariableRegistry();

		varreg.UpdateTimeIndices(mapTimeIxToNewTimeIx);
	}
*/
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::IndexVariableData(
	size_t sFileIxBegin,
	size_t sFileIxEnd
) {
	std::string strError;

	// Check if we're appending to an already populated FileListObject
	bool fAppendIndex =
		(m_vecVariableInfo.size() != 0) ||
		(m_vecDimensionInfo.size() != 0);

	// Open all files
	if (sFileIxBegin == InvalidFileIx) {
		sFileIxBegin = 0;
	}
	if (sFileIxEnd == InvalidFileIx) {
		sFileIxEnd = m_vecFilenames.size();
	}

	for (size_t f = sFileIxBegin; f < sFileIxEnd; f++) {

		// Open the NetCDF file
		std::string strFullFilename = m_strBaseDir + m_vecFilenames[f];
		NcFile ncFile(strFullFilename.c_str(), NcFile::ReadOnly);
		if (!ncFile.is_valid()) {
			return std::string("Unable to open data file \"")
				+ strFullFilename + std::string("\" for reading");
		}

		printf("Indexing %s\n", strFullFilename.c_str());

		// Load in global attributes
		strError = m_datainfo.FromNcFile(&ncFile, fAppendIndex, strFullFilename);
		if (strError != "") return strError;

		// time indices stored in this file
		std::vector<size_t> vecFileTimeIndices;

		// Find the time variable, if it exists
		NcVar * varTime = ncFile.get_var(m_strRecordDimName.c_str());
		if (varTime != NULL) {
			if (varTime->num_dims() != 1) {
				return std::string("\"")
					+ m_strRecordDimName
					+ std::string("\" variable must contain exactly one dimension in \"")
					+ m_vecFilenames[f] + std::string("\"");
			}
			if ((varTime->type() != ncInt) && (varTime->type() != ncDouble)) {
				return std::string("\"")
					+ m_strRecordDimName
					+ std::string("\" variable must be ncInt or ncDouble in \"")
					+ m_vecFilenames[f] + std::string("\"");
			}

			Time::CalendarType timecal;

			NcDim * dimTime = varTime->get_dim(0);
			if (dimTime == NULL) {
				_EXCEPTION1("Malformed NetCDF file \"%s\"",
					m_vecFilenames[f].c_str());
			}

			// Get calendar
			NcAtt * attTimeCalendar = varTime->get_att("calendar");
			if (attTimeCalendar == NULL) {
				timecal = Time::CalendarStandard;
			} else {
				std::string strTimeCalendar = attTimeCalendar->as_string(0);
				timecal = Time::CalendarTypeFromString(strTimeCalendar);
				if (timecal == Time::CalendarUnknown) {
					return std::string("Unknown calendar \"") + strTimeCalendar
						+ std::string("\" in \"")
						+ m_vecFilenames[f] + std::string("\"");
				}
			}

			// Get units attribute
			NcAtt * attTimeUnits = varTime->get_att("units");
			if (attTimeUnits != NULL) {
				m_strTimeUnits = attTimeUnits->as_string(0);
			}
			if (m_strTimeUnits == "") {
				return std::string("Unknown units for \"")
					+ m_strRecordDimName
					+ std::string("\" in \"")
					+ m_vecFilenames[f] + std::string("\"");
			}

			// Add Times to master array and store corresponding indices
			// in vecFileTimeIndices.
			DataArray1D<int> nTimes(dimTime->size());
			if (varTime->type() == ncInt) {
				varTime->set_cur((long)0);
				varTime->get(&(nTimes[0]), dimTime->size());
			}

			DataArray1D<double> dTimes(dimTime->size());
			if (varTime->type() == ncDouble) {
				varTime->set_cur((long)0);
				varTime->get(&(dTimes[0]), dimTime->size());
			}

			for (int t = 0; t < dimTime->size(); t++) {
				Time time(timecal);
				if (m_strTimeUnits != "") {
					if (varTime->type() == ncInt) {
						time.FromCFCompliantUnitsOffsetInt(
							m_strTimeUnits,
							nTimes[t]);

					} else if (varTime->type() == ncDouble) {
						time.FromCFCompliantUnitsOffsetDouble(
							m_strTimeUnits,
							dTimes[t]);
					}
				}

				std::map<Time, size_t>::const_iterator iterTime =
					m_mapTimeToIndex.find(time);

				if (iterTime == m_mapTimeToIndex.end()) {
					size_t sNewIndex = m_vecTimes.size();
					m_vecTimes.push_back(time);
					vecFileTimeIndices.push_back(sNewIndex);
					m_mapTimeToIndex.insert(
						std::pair<Time, size_t>(time, sNewIndex));
				} else {
					vecFileTimeIndices.push_back(iterTime->second);
				}
			}
		}

		printf("..File contains %lu times\n", vecFileTimeIndices.size());

		// Index all Dimensions
		printf("..Loading dimensions\n");
		const int nDims = ncFile.num_dims();
		for (int d = 0; d < nDims; d++) {
			NcDim * dim = ncFile.get_dim(d);
			std::string strDimName(dim->name());
			DimensionInfoMap::iterator iterDim =
				m_mapDimensionInfo.find(strDimName);

			//printf("....Dimension %i (%s)\n", d, strDimName.c_str());

			// New variable, not yet indexed
			bool fNewDimension = false;

			// Find the corresponding DimensionInfo structure
			size_t sDimIndex = 0;
			for (; sDimIndex < m_vecDimensionInfo.size(); sDimIndex++) {
				if (strDimName == m_vecDimensionInfo[sDimIndex]->m_strName) {
					break;
				}
			}
			if (sDimIndex == m_vecDimensionInfo.size()) {
				m_vecDimensionInfo.push_back(
					new DimensionInfo(strDimName));

				fNewDimension = true;
			}

			DimensionInfo & diminfo = *(m_vecDimensionInfo[sDimIndex]);

			// Store size
			if (fNewDimension) {
				diminfo.m_lSize = dim->size();
			}

			// Check for variable
			NcVar * varDim = ncFile.get_var(strDimName.c_str());
			if (varDim != NULL) {
				if (varDim->num_dims() != 1) {
					return std::string("ERROR: Dimension variable \"")
						+ varDim->name()
						+ std::string("\" must have exactly 1 dimension");
				}
				if (std::string(varDim->get_dim(0)->name()) != strDimName) {
					return std::string("ERROR: Dimension variable \"")
						+ varDim->name()
						+ std::string("\" does not have dimension \"");
						+ varDim->name()
						+ std::string("\"");
				}

				// Initialize the DataObjectInfo from the NcVar
				strError = diminfo.FromNcVar(varDim, !fNewDimension);
				if (strError != "") return strError;

				// Get the values from the dimension
				if (fNewDimension) {
					if (diminfo.m_nctype == ncDouble) {
						diminfo.m_dValuesDouble.resize(diminfo.m_lSize);
						varDim->set_cur((long)0);
						varDim->get(&(diminfo.m_dValuesDouble[0]), diminfo.m_lSize);
					} else if (diminfo.m_nctype == ncFloat) {
						diminfo.m_dValuesFloat.resize(diminfo.m_lSize);
						varDim->set_cur((long)0);
						varDim->get(&(diminfo.m_dValuesFloat[0]), diminfo.m_lSize);
					} else {
						_EXCEPTIONT("Unsupported dimension nctype");
					}
				}
/*
				// Dimension is of type vertical
				if ((strDimName == "lev") ||
					(strDimName == "pres") ||
					(strDimName == "z") ||
					(strDimName == "plev")
				) {
					diminfo.m_eType = DimensionInfo::Type_Vertical;

					// Determine order of dimension

					// Find the variable associated with this dimension
					if (varDim != NULL) {

						// Check for presence of "positive" attribute
						NcAtt * attPositive = varDim->get_att("positive");
						if (attPositive != NULL) {
							if (std::string("down") == attPositive->as_string(0)) {
								diminfo.m_nOrder = (-1);
							}

						// Obtain orientation from values
						} else {

							// Dimension variable is of type double
							if (varDim->type() == ncDouble) {

								// Positive orientation (bottom-up)
								if (diminfo.m_dValues[1] > diminfo.m_dValues[0]) {
									diminfo.m_nOrder = (+1);
									for (size_t s = 0; s < diminfo.m_dValues.size()-1; s++) {
										if (diminfo.m_dValues[s+1] < diminfo.m_dValues[s]) {
											_EXCEPTION1("Dimension variable \"%s\" is not monotone",
												strDimName.c_str());
										}
									}

								// Negative orientation (top-down)
								} else {
									diminfo.m_nOrder = (-1);
									for (size_t s = 0; s < diminfo.m_dValues.size()-1; s++) {
										if (diminfo.m_dValues[s+1] > diminfo.m_dValues[s]) {
											_EXCEPTION1("Dimension variable \"%s\" is not monotone",
												strDimName.c_str());
										}
									}
								}

							} else {
								_EXCEPTION1("Unknown type for dimension variable \"%s\"",
									strDimName.c_str());
							}
						}
					}

				// Record dimension
				} else if (strDimName == m_strRecordDimName) {
					diminfo.m_eType = DimensionInfo::Type_Record;

				// Auxiliary dimension
				} else {
					diminfo.m_eType = DimensionInfo::Type_Auxiliary;
				}

				// Insert dimension into dimension info map
				m_mapDimensionInfo.insert(
					DimensionInfoMap::value_type(
						strDimName, diminfo));

			} else if (iterDim->second.m_lSize != dim->size()) {
				_EXCEPTIONT("Inconsistent dimension sizes");
*/
			}
		}

		// Loop over all Variables
		printf("..Loading variables\n");
		const int nVariables = ncFile.num_vars();
		for (int v = 0; v < nVariables; v++) {
			NcVar * var = ncFile.get_var(v);
			if (var == NULL) {
				_EXCEPTION1("Malformed NetCDF file \"%s\"",
					m_vecFilenames[f].c_str());
			}

			std::string strVariableName = var->name();

			// Don't index dimension variables
			bool fDimensionVar = false;
			for (int d = 0; d < m_vecDimensionInfo.size(); d++) {
				if (m_vecDimensionInfo[d]->m_strName == strVariableName) {
					fDimensionVar = true;
					break;
				}
			}
			if (fDimensionVar) {
				continue;
			}

			//printf("....Variable %i (%s)\n", v, strVariableName.c_str());

			// New variable, not yet indexed
			bool fNewVariable = false;

			// Find the corresponding VariableInfo structure
			size_t sVarIndex = 0;
			for (; sVarIndex < m_vecVariableInfo.size(); sVarIndex++) {
				if (strVariableName == m_vecVariableInfo[sVarIndex]->m_strName) {
					break;
				}
			}
			if (sVarIndex == m_vecVariableInfo.size()) {
				m_vecVariableInfo.push_back(
					new VariableInfo(strVariableName));

				fNewVariable = true;
			}

			VariableInfo & info = *(m_vecVariableInfo[sVarIndex]);

			// Initialize the DataObjectInfo from the NcVar
			strError = info.FromNcVar(var, !fNewVariable);
			if (strError != "") return strError;

			// Load dimension information
			const int nDims = var->num_dims();
/*
			if (info.m_vecDimSizes.size() != 0) {
				if (info.m_vecDimSizes.size() != nDims) {
					return std::string("Variable \"") + strVariableName
						+ std::string("\" has inconsistent dimensionality across files");
				}
			}
*/
			info.m_vecDimNames.resize(nDims);
			info.m_vecDimSizes.resize(nDims);
			for (int d = 0; d < nDims; d++) {
				info.m_vecDimNames[d] = var->get_dim(d)->name();

				if (info.m_vecDimNames[d] == m_strRecordDimName) {
					if (info.m_iTimeDimIx == (-1)) {
						info.m_iTimeDimIx = d;
					} else if (info.m_iTimeDimIx != d) {
						return std::string("ERROR: Variable \"") + strVariableName
							+ std::string("\" has inconsistent \"time\" dimension across files");
					}
					info.m_vecDimSizes[d] = (-1);
				} else {
					info.m_vecDimSizes[d] = var->get_dim(d)->size();
				}

				if ((info.m_vecDimNames[d] == "lev") ||
					(info.m_vecDimNames[d] == "pres") ||
					(info.m_vecDimNames[d] == "z") ||
					(info.m_vecDimNames[d] == "plev")
				) {
					if (info.m_iVerticalDimIx != (-1)) {
						if (info.m_iVerticalDimIx != d) {
							return std::string("ERROR: Possibly multiple vertical dimensions in variable ")
								+ info.m_strName;
						}
					}
					info.m_iVerticalDimIx = d;
				}
			}
/*
			// Determine direction of vertical dimension
			if (info.m_iVerticalDimIx != (-1)) {
				DimensionInfoMap::iterator iterDimInfo =
					m_mapDimensionInfo.find(info.m_vecDimNames[info.m_iVerticalDimIx]);

				if (iterDimInfo != m_mapDimensionInfo.end()) {
					info.m_nVerticalDimOrder = iterDimInfo->second.m_nOrder;

				} else {
					_EXCEPTIONT("Logic error");
				}
			}
*/
			// No time information on this Variable
			if (info.m_iTimeDimIx == (-1)) {
				if (info.m_mapTimeFile.size() == 0) {
					info.m_mapTimeFile.insert(
						std::pair<size_t, LocalFileTimePair>(
							InvalidTimeIx,
							LocalFileTimePair(f, 0)));

				} else if (info.m_mapTimeFile.size() == 1) {
					VariableTimeFileMap::const_iterator iterTimeFile =
						info.m_mapTimeFile.begin();

					if (iterTimeFile->first != InvalidTimeIx) {
						return std::string("Variable \"") + strVariableName
							+ std::string("\" has inconsistent \"time\" dimension across files");
					}

				} else {
					return std::string("Variable \"") + strVariableName
						+ std::string("\" has inconsistent \"time\" dimension across files");
				}

			// Add file and time indices to VariableInfo
			} else {
				for (int t = 0; t < vecFileTimeIndices.size(); t++) {
					VariableTimeFileMap::const_iterator iterTimeFile =
						info.m_mapTimeFile.find(vecFileTimeIndices[t]);

					if (iterTimeFile == info.m_mapTimeFile.end()) {
						info.m_mapTimeFile.insert(
							std::pair<size_t, LocalFileTimePair>(
								vecFileTimeIndices[t],
								LocalFileTimePair(f, t)));

					} else {
						return std::string("Variable \"") + strVariableName
							+ std::string("\" has repeated time across files:\n")
							+ std::string("Time: ") + m_vecTimes[vecFileTimeIndices[t]].ToString() + std::string("\n")
							+ std::string("File1: ") + m_vecFilenames[iterTimeFile->second.first] + std::string("\n")
							+ std::string("File2: ") + m_vecFilenames[f];
					}
				}
			}
		}
	}

	// Sort the Time array
	SortTimeArray();

	for (int v = 0; v < m_vecVariableInfo.size(); v++) {

		VariableInfo & varinfo = *(m_vecVariableInfo[v]);

		// Update the time dimension size for all variables
		int iTimeDimIx = varinfo.m_iTimeDimIx;
		if (iTimeDimIx != (-1)) {
			if (varinfo.m_vecDimSizes.size() < iTimeDimIx) {
				_EXCEPTIONT("Logic error");
			}
			varinfo.m_vecDimSizes[iTimeDimIx] =
				varinfo.m_mapTimeFile.size();
		}

		// Initialize auxiliary dimension information for all variables
		varinfo.m_vecAuxDimNames.clear();
		varinfo.m_vecAuxDimSizes.clear();
		for (size_t d = 0; d < varinfo.m_vecDimSizes.size(); d++) {
			bool fFound = false;
			for (int g = 0; g < m_vecGridDimNames.size(); g++) {
				if (m_vecGridDimNames[g] == varinfo.m_vecDimNames[d]) {
					fFound = true;
					break;
				}
			}
			if (!fFound) {
				varinfo.m_vecAuxDimNames.push_back(
					varinfo.m_vecDimNames[d]);
				varinfo.m_vecAuxDimSizes.push_back(
					varinfo.m_vecDimSizes[d]);
			}
		}
	}

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::OutputTimeVariableIndexCSV(
	const std::string & strCSVOutputFilename
) {
#if defined(HYPERION_MPIOMP)
	// Only output on root thread
	int nRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &nRank);
	if (nRank != 0) {
		return std::string("");
	}
#endif

	if (m_vecTimes.size() != m_mapTimeToIndex.size()) {
		_EXCEPTIONT("vecTimes / mapTimeToIndex mismatch");
	}

	std::vector< std::pair<size_t,int> > iTimeVariableIndex;
	iTimeVariableIndex.resize(m_vecVariableInfo.size());

	// Open output file
	std::ofstream ofOutput(strCSVOutputFilename.c_str());
	if (!ofOutput.is_open()) {
		return std::string("Unable to open output file \"") + strCSVOutputFilename + "\"";
	}

	// Output variables across header
	ofOutput << "time";
	for (int v = 0; v < m_vecVariableInfo.size(); v++) {
		ofOutput << "," << m_vecVariableInfo[v]->m_strName;
	}
	ofOutput << std::endl;

	// Output variables with no time dimension
	ofOutput << "NONE";
	for (size_t v = 0; v < m_vecVariableInfo.size(); v++) {
		if (m_vecVariableInfo[v]->m_iTimeDimIx == (-1)) {
			ofOutput << ",X";
		} else {
			ofOutput << ",";
		}
	}
	ofOutput << std::endl;

	// Output variables with time dimension
	for (size_t t = 0; t < m_vecTimes.size(); t++) {
		ofOutput << m_vecTimes[t].ToString();

		for (size_t v = 0; v < m_vecVariableInfo.size(); v++) {

			VariableTimeFileMap::const_iterator iterTimeFile =
				m_vecVariableInfo[v]->m_mapTimeFile.find(t);

			if (iterTimeFile == m_vecVariableInfo[v]->m_mapTimeFile.end()) {
				ofOutput << ",";
			} else {
				ofOutput << "," << iterTimeFile->second.first
					<< ":" << iterTimeFile->second.second;
			}
		}
		ofOutput << std::endl;
	}

	// Output file names
	ofOutput << std::endl << std::endl;

	ofOutput << "file_ix,filename" << std::endl;
	for (size_t f = 0; f < m_vecFilenames.size(); f++) {
		ofOutput << f << ",\"" << m_strBaseDir << m_vecFilenames[f] << "\"" << std::endl;
	}

	return ("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::OutputTimeVariableIndexXML(
	const std::string & strXMLOutputFilename
) {
	using namespace tinyxml2;
	/*
	XMLDocument xmlDoc;
	XMLError eResult = xmlDoc.LoadFile("charles.xml");
	*/
	tinyxml2::XMLDocument xmlDoc;

	// Declaration
	xmlDoc.InsertEndChild(
		xmlDoc.NewDeclaration(
			"xml version=\"1.0\" encoding=\"\""));

	// DOCTYPE
	tinyxml2::XMLUnknown * pdoctype = xmlDoc.NewUnknown("DOCTYPE dataset SYSTEM \"http://www-pcmdi.llnl.gov/software/cdms/cdml.dtd\"");
	xmlDoc.InsertEndChild(pdoctype);

	// Dataset 
	tinyxml2::XMLElement * pdata = xmlDoc.NewElement("dataset");
	{
		AttributeMap::const_iterator iterAttKey =
			m_datainfo.m_mapKeyAttributes.begin();
		for (; iterAttKey != m_datainfo.m_mapKeyAttributes.end(); iterAttKey++) {
			pdata->SetAttribute(iterAttKey->first.c_str(), iterAttKey->second.c_str());
		}

		AttributeMap::const_iterator iterAttOther =
			m_datainfo.m_mapOtherAttributes.begin();
		for (; iterAttOther != m_datainfo.m_mapOtherAttributes.end(); iterAttOther++) {
			tinyxml2::XMLElement * pattr = xmlDoc.NewElement("attr");
			pattr->SetAttribute("name", iterAttOther->first.c_str());
			pattr->SetAttribute("datatype", "String");
			pattr->SetText(iterAttOther->second.c_str());
			pdata->InsertEndChild(pattr);
		}
	}
	xmlDoc.InsertEndChild(pdata);

	// Output dimensions
	for (int d = 0; d < m_vecDimensionInfo.size(); d++) {
		const DimensionInfo * pdiminfo = m_vecDimensionInfo[d];

		tinyxml2::XMLElement * pdim = xmlDoc.NewElement("axis");
		pdim->SetAttribute("id", pdiminfo->m_strName.c_str());
		pdim->SetAttribute("units", pdiminfo->m_strUnits.c_str());
		pdim->SetAttribute("length", (int64_t)pdiminfo->m_lSize);
		pdim->SetAttribute("datatype", NcTypeToString(pdiminfo->m_nctype).c_str());

		AttributeMap::const_iterator iterAttKey =
			pdiminfo->m_mapKeyAttributes.begin();
		for (; iterAttKey != pdiminfo->m_mapKeyAttributes.end(); iterAttKey++) {
			pdim->SetAttribute(iterAttKey->first.c_str(), iterAttKey->second.c_str());
		}

		AttributeMap::const_iterator iterAttOther =
			pdiminfo->m_mapOtherAttributes.begin();
		for (; iterAttOther != pdiminfo->m_mapOtherAttributes.end(); iterAttOther++) {
			tinyxml2::XMLElement * pattr = xmlDoc.NewElement("attr");
			pattr->SetAttribute("name", iterAttOther->first.c_str());
			pattr->SetAttribute("datatype", "String");
			pattr->SetText(iterAttOther->second.c_str());
			pdim->InsertEndChild(pattr);
		}

		bool fHasValues = false;
		if ((pdiminfo->m_nctype == ncDouble) && (pdiminfo->m_dValuesDouble.size() != 0)) {
			fHasValues = true;
		}
		if ((pdiminfo->m_nctype == ncFloat) && (pdiminfo->m_dValuesFloat.size() != 0)) {
			fHasValues = true;
		}

		//std::cout << pdiminfo->m_strName << " " << pdiminfo->m_dValuesFloat.size() << std::endl;
		if (fHasValues) {
			std::ostringstream ssText;

			// Double type
			if (pdiminfo->m_nctype == ncDouble) {
				ssText << std::setprecision(17);
				ssText << "[";
				for (int i = 0; i < pdiminfo->m_dValuesDouble.size(); i++) {
					ssText << pdiminfo->m_dValuesDouble[i];
					if (i != pdiminfo->m_dValuesDouble.size()-1) {
						ssText << " ";
					}
				}
				ssText << "]";

			// Float type
			} else if (pdiminfo->m_nctype == ncFloat) {
				ssText << std::setprecision(8);
				ssText << "[";
				for (int i = 0; i < pdiminfo->m_dValuesFloat.size(); i++) {
					ssText << pdiminfo->m_dValuesFloat[i];
					if (i != pdiminfo->m_dValuesFloat.size()-1) {
						ssText << " ";
					}
				}
				ssText << "]";
			} 

			pdim->SetText(ssText.str().c_str());
		}

		pdata->InsertEndChild(pdim);
	}

	// Output variables
	for (int v = 0; v < m_vecVariableInfo.size(); v++) {
		const VariableInfo * pvarinfo = m_vecVariableInfo[v];

		tinyxml2::XMLElement * pvar = xmlDoc.NewElement("variable");
		pvar->SetAttribute("id", pvarinfo->m_strName.c_str());
		pvar->SetAttribute("datatype", NcTypeToString(pvarinfo->m_nctype).c_str());
		pvar->SetAttribute("units", pvarinfo->m_strUnits.c_str());

		AttributeMap::const_iterator iterAttKey =
			pvarinfo->m_mapKeyAttributes.begin();
		for (; iterAttKey != pvarinfo->m_mapKeyAttributes.end(); iterAttKey++) {
			pvar->SetAttribute(iterAttKey->first.c_str(), iterAttKey->second.c_str());
		}

		AttributeMap::const_iterator iterAttOther =
			pvarinfo->m_mapOtherAttributes.begin();
		for (; iterAttOther != pvarinfo->m_mapOtherAttributes.end(); iterAttOther++) {
			tinyxml2::XMLElement * pattr = xmlDoc.NewElement("attr");
			pattr->SetAttribute("name", iterAttOther->first.c_str());
			pattr->SetAttribute("datatype", "String");
			pattr->SetText(iterAttOther->second.c_str());
			pvar->InsertEndChild(pattr);
		}

		if (m_vecVariableInfo[v]->m_vecDimNames.size() != 0) {
			tinyxml2::XMLElement * pvardom = xmlDoc.NewElement("domain");
			pvar->InsertEndChild(pvardom);

			for (int d = 0; d < m_vecVariableInfo[v]->m_vecDimNames.size(); d++) {
				tinyxml2::XMLElement * pvardomelem = xmlDoc.NewElement("domElem");
				pvardomelem->SetAttribute("name", pvarinfo->m_vecDimNames[d].c_str());
				pvardom->InsertEndChild(pvardomelem);
			}
		}

		pdata->InsertEndChild(pvar);
	}

	xmlDoc.SaveFile(strXMLOutputFilename.c_str());

	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

std::string FileListObject::OutputTimeVariableIndexJSON(
	const std::string & strJSONOutputFilename
) {
	return std::string("");
}

///////////////////////////////////////////////////////////////////////////////

