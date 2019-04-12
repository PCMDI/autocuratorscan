///////////////////////////////////////////////////////////////////////////////
///
///	\file    FileListObject.h
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

#ifndef _FILELISTOBJECT_H_
#define _FILELISTOBJECT_H_

#include "Announce.h"
#include "Object.h"
#include "TimeObj.h"
#include "DataArray1D.h"
#include "GlobalFunction.h"
#include "netcdfcpp.h"

///////////////////////////////////////////////////////////////////////////////

class RecapConfigObject;

class GridObject;

class Variable;

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A local (file,time) pair.
///	</summary>
typedef std::pair<size_t, int> LocalFileTimePair;

///	<summary>
///		A map from time indices to local (file,time) pairs.
///	</summary>
typedef std::map<size_t, LocalFileTimePair> VariableTimeFileMap;

///	<summary>
///		A map from attribute names to values.
///	</summary>
typedef std::map<std::string, std::string> AttributeMap;

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class for storing metadata about a data object from a FileList.
///	</summary>
class DataObjectInfo {

public:
	///	<summary>
	///		Default constructor.
	///	</summary>
	DataObjectInfo() :
		m_nctype(ncNoType)
	{ }

	///	<summary>
	///		Constructor.
	///	</summary>
	DataObjectInfo(
		const std::string & strName
	) :
		m_strName(strName),
		m_nctype(ncNoType)
	{ }

public:
	///	<summary>
	///		Populate from a NcFile.
	///	</summary>
	std::string FromNcFile(
		NcFile * ncfile,
		bool fCheckConsistency,
		const std::string & strFilename
	);

	///	<summary>
	///		Populate from a NcVar.
	///	</summary>
	std::string FromNcVar(
		NcVar * var,
		bool fCheckConsistency
	);

public:
	///	<summary>
	///		Equality operator.
	///	</summary>
	bool operator== (const DataObjectInfo & info) const {
		return (
			(m_strName == info.m_strName) &&
			(m_nctype == info.m_nctype) &&
			(m_strUnits == info.m_strUnits) &&
			(m_mapKeyAttributes == info.m_mapKeyAttributes) &&
			(m_mapOtherAttributes == info.m_mapOtherAttributes));
	}

	///	<summary>
	///		Inequality operator.
	///	</summary>
	bool operator!= (const DataObjectInfo & info) const {
		return !((*this) == info);
	}

public:
	///	<summary>
	///		Data object name.
	///	</summary>
	std::string m_strName;

	///	<summary>
	///		NcType for the Variable.
	///	</summary>
	NcType m_nctype;

	///	<summary>
	///		Units for the Variable.
	///	</summary>
	std::string m_strUnits;

	///	<summary>
	///		Key attributes for this Variable.
	///	</summary>
	AttributeMap m_mapKeyAttributes;

	///	<summary>
	///		Other attributes for this Variable.
	///	</summary>
	AttributeMap m_mapOtherAttributes;

};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that describes primitive variable information from a FileList.
///	</summary>
class VariableInfo : public DataObjectInfo {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	VariableInfo(
		const std::string & strName
	) :
		DataObjectInfo(strName),
		m_iTimeDimIx(-1),
		m_iVerticalDimIx(-1),
		m_nVerticalDimOrder(+1)
	{ } 

public:
	///	<summary>
	///		Index of time dimension or (-1) if time dimension doesn't exist.
	///	</summary>
	int m_iTimeDimIx;

	///	<summary>
	///		Index of the vertical dimension or (-1) if vertical dimension doesn't exist.
	///	</summary>
	int m_iVerticalDimIx;

	///	<summary>
	///		(+1) if the vertical coordinate is bottom-up, (-1) if top-down.
	///	</summary>
	int m_nVerticalDimOrder;

	///	<summary>
	///		Dimension names.
	///	</summary>
	std::vector<std::string> m_vecDimNames;

	///	<summary>
	///		Size of each dimension.
	///	</summary>
	std::vector<long> m_vecDimSizes;

	///	<summary>
	///		Auxiliary dimension names.
	///	</summary>
	std::vector<std::string> m_vecAuxDimNames;

	///	<summary>
	///		Size of each auxiliary dimension.
	///	</summary>
	std::vector<long> m_vecAuxDimSizes;

	///	<summary>
	///		Map from Times to filename index and time index.
	///	</summary>
	VariableTimeFileMap m_mapTimeFile;
};

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that describes dimension information from a FileList.
///	</summary>
class DimensionInfo : public DataObjectInfo {

public:
	enum Type {
		Type_Unknown = (-1),
		Type_Auxiliary = 0,
		Type_Grid = 1,
		Type_Record = 2,
		Type_Vertical = 3
	};

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	DimensionInfo() :
		DataObjectInfo(""),
		m_lSize(0),
		m_nOrder(0),
		m_eType(Type_Unknown)
	{ }

	///	<summary>
	///		Constructor.
	///	</summary>
	DimensionInfo(
		const std::string & strName
	) :
		DataObjectInfo(strName),
		m_lSize(0),
		m_nOrder(0),
		m_eType(Type_Unknown)
	{ }

public:
	///	<summary>
	///		Equality operator.
	///	</summary>
	bool operator== (const DimensionInfo & diminfo) const {
		return (
			((DataObjectInfo &)(*this) == (DataObjectInfo &)(diminfo)) &&
			(m_eType == diminfo.m_eType) &&
			(m_lSize == diminfo.m_lSize) &&
			(m_nOrder == diminfo.m_nOrder) &&
			(m_dValuesFloat == diminfo.m_dValuesFloat) &&
			(m_dValuesDouble == diminfo.m_dValuesDouble));
	}

	///	<summary>
	///		Inequality operator.
	///	</summary>
	bool operator!= (const DimensionInfo & diminfo) const {
		return !((*this) == diminfo);
	}

	///	<summary>
	///		Convert to string.
	///	</summary>
	std::string ToString() const {
		std::string str;
		str += m_strName + " : ";
		str += std::to_string(m_eType) + " : ";
		str += std::to_string(m_lSize) + " : ";
		str += std::to_string(m_nOrder) + " : ";
		str += m_strUnits + "\n";
		str += "[";

		if (m_nctype == ncDouble) {
			for (int i = 0; i < m_dValuesDouble.size(); i++) {
				str += std::to_string(m_dValuesDouble[i]);
				if (i != m_dValuesDouble.size()-1) {
					str += ", ";
				}
			}

		} else if (m_nctype == ncFloat) {
			for (int i = 0; i < m_dValuesFloat.size(); i++) {
				str += std::to_string(m_dValuesFloat[i]);
				if (i != m_dValuesFloat.size()-1) {
					str += ", ";
				}
			}
		}

		str += "]";

		return str;
	}

public:
	///	<summary>
	///		Dimension type.
	///	</summary>
	Type m_eType;

	///	<summary>
	///		Dimension size.
	///	</summary>
	long m_lSize;

	///	<summary>
	///		Dimension order.
	///	</summary>
	int m_nOrder;

	///	<summary>
	///		Dimension values as floats.
	///	</summary>
	std::vector<float> m_dValuesFloat;

	///	<summary>
	///		Dimension values as doubles.
	///	</summary>
	std::vector<double> m_dValuesDouble;
};

///	<summary>
///		A map from a dimension name to DimensionInfo structure.
///	</summary>
typedef std::map<std::string, DimensionInfo> DimensionInfoMap;

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A GlobalFunction that builds a new FileListObject.
///	</summary>
class FileListObjectConstructor : public GlobalFunction {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	FileListObjectConstructor(const std::string & strName) :
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
///		A data structure describing a list of files.
///	</summary>
class FileListObject : public Object {

public:
	///	<summary>
	///		Invalid File index.
	///	</summary>
	static const size_t InvalidFileIx;

	///	<summary>
	///		Invalid Time index.
	///	</summary>
	static const size_t InvalidTimeIx;

	///	<summary>
	///		A value to denote that a dimension has inconsistent sizes
	///		across files.
	///	</summary>
	static const long InconsistentDimensionSizes;

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	FileListObject(
		const std::string & strName
	) :
		Object(strName),
		m_pobjRecapConfig(NULL),
		m_strRecordDimName("time"),
		m_sReduceTargetIx(InvalidFileIx)
	{ }

	///	<summary>
	///		Destructor.
	///	</summary>
	~FileListObject();

	///	<summary>
	///		Call a member function of this Object.
	///	</summary>
	virtual std::string Call(
		const ObjectRegistry & objreg,
		const std::string & strFunctionName,
		const std::vector<std::string> & vecCommandLine,
		const std::vector<ObjectType> & vecCommandLineType,
		Object ** ppReturn
	);

	///	<summary>
	///		Set the RecapConfigObject pointer for this FileListObject.
	///	</summary>
	void SetRecapConfigObject(
		RecapConfigObject * pobjRecapConfig
	) {
		m_pobjRecapConfig = pobjRecapConfig;
	}

public:
	///	<summary>
	///		Get the count of filenames.
	///	</summary>
	size_t GetFilenameCount() const {
		return m_vecFilenames.size();
	}

	///	<summary>
	///		Get the vector of filenames.
	///	</summary>
	const std::string & GetFilename(size_t f) const {
		if (f >= m_vecFilenames.size()) {
			_EXCEPTIONT("Index out of range");
		}
		return m_vecFilenames[f];
	}

	///	<summary>
	///		Get the VariableInfo associated with a given variable name.
	///	</summary>
	const VariableInfo * GetVariableInfo(
		const std::string & strVariableName
	) const {
		for (size_t i = 0; i < m_vecVariableInfo.size(); i++) {
			if (m_vecVariableInfo[i]->m_strName == strVariableName) {
				return (m_vecVariableInfo[i]);
			}
		}
		return NULL;
	}

	///	<summary>
	///		Populate from a search string.
	///	</summary>
	std::string PopulateFromSearchString(
		const std::string & strSearchString
	);

	///	<summary>
	///		Add a series of files with the given filename template.
	///	</summary>
	std::string CreateFilesFromTemplate(
		const std::string & strFilenameTemplate,
		const GridObject * pobjGrid,
		int nTimesPerFile
	);

	///	<summary>
	///		Add a single timeslice file with the given filename.
	///	</summary>
	std::string CreateFileNoTime(
		const std::string & strFilename,
		const GridObject * pobjGrid
	);

	///	<summary>
	///		Set the reduce target by filename.
	///	</summary>
	std::string SetReduceTarget(
		const std::string & strTargetFilename
	);

	///	<summary>
	///		Check if the FileList has a reduce target.
	///	</summary>
	bool HasReduceTarget() const {
		if (m_sReduceTargetIx != InvalidFileIx) {
			return true;
		}
		return false;
	}

public:
	///	<summary>
	///		Get the record dimension name.
	///	</summary>
	const std::string & GetRecordDimName() const {
		return m_strRecordDimName;
	}

	///	<summary>
	///		Get the number of time indices in the file list.
	///	</summary>
	size_t GetTimeCount() const {
		return m_vecTimes.size();
	}

	///	<summary>
	///		Get the Time with the specified index.
	///	</summary>
	const Time & GetTime(int iTime) const {
		if (iTime >= m_vecTimes.size()) {
			_EXCEPTIONT("Out of range");
		}
		return m_vecTimes[iTime];
	}

	///	<summary>
	///		Get the vector of Times associated with the FileList.
	///	</summary>
	const std::vector<Time> & GetTimes() const {
		return m_vecTimes;
	}

	///	<summary>
	///		Get the information on the specified dimension.
	///	</summary>
	const DimensionInfo & GetDimInfo(
		const std::string & strDimName
	) const {
		DimensionInfoMap::const_iterator iterDimInfo =
			m_mapDimensionInfo.find(strDimName);
		if (iterDimInfo == m_mapDimensionInfo.end()) {
			_EXCEPTIONT("Invalid dimension");
		}
		return (iterDimInfo->second);
	}

	///	<summary>
	///		Get the size of the specified dimension.
	///	</summary>
	long GetDimSize(const std::string & strDimName) const {
		DimensionInfoMap::const_iterator iter =
			m_mapDimensionInfo.find(strDimName);

		if (iter != m_mapDimensionInfo.end()) {
			return iter->second.m_lSize;
		}

		_EXCEPTIONT("Invalid dimension name");
	}

	///	<summary>
	///		Check if another FileListObject has a compatible set of
	///		Times indices.
	///	</summary>
	bool IsCompatible(
		const FileListObject * pobjFileList
	);

	///	<summary>
	///		Distribute available time indices across MPI ranks.
	///	</summary>
	void GetOnRankTimeIndices(
		std::vector<size_t> & vecTimeIndices,
		size_t sTimeStride = 1
	);

	///	<summary>
	///		Load the data from a particular variable into the given array.
	///	</summary>
	std::string LoadData_float(
		const std::string & strVariableName,
		const std::vector<long> & vecAuxIndices,
		DataArray1D<float> & data
	);

	///	<summary>
	///		Write the data from the given array to disk.
	///	</summary>
	std::string WriteData_float(
		const std::string & strVariableName,
		const std::vector<long> & vecAuxIndices,
		const DataArray1D<float> & data
	);

	///	<summary>
	///		Add a new variable from a template.
	///	</summary>
	std::string AddVariableFromTemplate(
		const FileListObject * pobjSourceFileList,
		const Variable * pvar,
		VariableInfo ** ppvarinfo
	);

	///	<summary>
	///		Add a new variable from a template and replace the vertical dimension.
	///	</summary>
	std::string AddVariableFromTemplateWithNewVerticalDim(
		const FileListObject * pobjSourceFileList,
		const Variable * pvar,
		const std::string & strVerticalDimName,
		VariableInfo ** ppvarinfo
	);

public:
	///	<summary>
	///		Add the given dimension to this FileListObject.
	///	</summary>
	std::string AddDimension(
		const std::string & strDimName,
		long lDimSize,
		DimensionInfo::Type eDimType
	);

	///	<summary>
	///		Add the given vertical dimension to this FileListObject.
	///	</summary>
	std::string AddVerticalDimension(
		const std::string & strDimName,
		const std::vector<double> & vecDimValues,
		const std::string & strDimUnits
	);

	///	<summary>
	///		Get the size of the specified dimension.
	///	</summary>
	long GetDimensionSize(
		const std::string & strDimName
	) const;

protected:
	///	<summary>
	///		Sort the array of Times to keep m_vecTimes in
	///		chronological order.
	///	</summary>
	void SortTimeArray();

	///	<summary>
	///		Index variable data.
	///	</summary>
	std::string IndexVariableData(
		size_t sFileIxBegin = InvalidFileIx,
		size_t sFileIxEnd = InvalidFileIx
	);

public:
	///	<summary>
	///		Output the time-variable index as a CSV.
	///	</summary>
	std::string OutputTimeVariableIndexCSV(
		const std::string & strCSVOutput
	);

	///	<summary>
	///		Output the time-variable index as a XML.
	///	</summary>
	std::string OutputTimeVariableIndexXML(
		const std::string & strXMLOutput
	);

	///	<summary>
	///		Output the time-variable index as a JSON.
	///	</summary>
	std::string OutputTimeVariableIndexJSON(
		const std::string & strJSONOutput
	);

protected:
	///	<summary>
	///		Pointer to the associated RecapConfigObject.
	///	</summary>
	RecapConfigObject * m_pobjRecapConfig;

	///	<summary>
	///		The DataObjectInfo describing this global dataset.
	///	</summary>
	DataObjectInfo m_datainfo;

	///	<summary>
	///		The name of the record dimension (default "time")
	///	</summary>
	std::string m_strRecordDimName;

	///	<summary>
	///		The base directory.
	///	</summary>
	std::string m_strBaseDir;

	///	<summary>
	///		The list of filenames.
	///	</summary>
	std::vector<std::string> m_vecFilenames;

	///	<summary>
	///		The format of the record variable.
	///	</summary>
	std::string m_strTimeUnits;

	///	<summary>
	///		The list of Times that appear in the FileList
	///		(in chronological order).
	///	</summary>
	std::vector<Time> m_vecTimes;

	///	<summary>
	///		A map from Time to m_vecTimes vector index
	///	</summary>
	std::map<Time, size_t> m_mapTimeToIndex;

	///	<summary>
	///		Information on variables that appear in the FileList.
	///	</summary>
	std::vector<VariableInfo *> m_vecVariableInfo;

	///	<summary>
	///		Information on variables that appear in the FileList.
	///	</summary>
	std::vector<DimensionInfo *> m_vecDimensionInfo;

	///	<summary>
	///		A set containing dimension information for this FileList.
	///	</summary>
	DimensionInfoMap m_mapDimensionInfo;

	///	<summary>
	///		Names of grid dimensions for this FileList.
	///	</summary>
	std::vector<std::string> m_vecGridDimNames;

	///	<summary>
	///		Filename index that is the target of reductions (output mode).
	///	</summary>
	size_t m_sReduceTargetIx;

	///	<summary>
	///		Filename index for each of the time indices (output mode).
	///	</summary>
	std::map<size_t, LocalFileTimePair> m_mapOutputTimeFile;
};

///////////////////////////////////////////////////////////////////////////////

#endif

