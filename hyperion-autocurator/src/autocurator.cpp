///////////////////////////////////////////////////////////////////////////////
///
///	\file    autocurator.cpp
///	\author  Paul Ullrich
///	\version January 31, 2019
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

#include "CommandLine.h"
#include "Announce.h"
#include "Object.h"
#include "FileListObject.h"

#include <string>

#include "netcdfcpp.h"

#if defined(HYPERION_MPIOMP)
#include <mpi.h>
#endif

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

#if defined(HYPERION_MPIOMP)
	// Initialize MPI
	MPI_Init(&argc, &argv);
#endif

	// Turn off fatal errors in NetCDF
	NcError error(NcError::silent_nonfatal);

try {

	// Set Announce to only output on head node
	AnnounceOnlyOutputOnRankZero();

	// Path for files
	std::string strFilePath;

	// Output CSV file
	std::string strOutputFile;

	// Parse the command line
	BeginCommandLine()
   	CommandLineString(strFilePath, "files", "");
	CommandLineString(strOutputFile, "out", "");

	ParseCommandLine(argc, argv);
	EndCommandLine(argv)

	// Create a new FileListObject
	AnnounceStartBlock("Creating FileListObject");
	FileListObject objFileList("file_list");
	AnnounceEndBlock("Done");

	// Populate from search string
	AnnounceStartBlock("Populating FileListObject\n");
	std::string strError = objFileList.PopulateFromSearchString(strFilePath);
	if (strError != "") {
		std::cout << strError << std::endl;
		return (-1);
	}
	AnnounceEndBlock("Done");
/*
	// Output to CSV file
	AnnounceStartBlock("Output to CSV file\n");
	objFileList.OutputTimeVariableIndexCSV(strOutputFile);
	AnnounceEndBlock("Done");
*/
	// Output to XML file
	AnnounceStartBlock("Output to XML file\n");
	objFileList.OutputTimeVariableIndexXML(strOutputFile);
	AnnounceEndBlock("Done");

} catch(Exception & e) {
	Announce(e.ToString().c_str());
} catch(...) {
}

#if defined(HYPERION_MPIOMP)
	// Deinitialize MPI
	MPI_Finalize();
#endif
}

///////////////////////////////////////////////////////////////////////////////

