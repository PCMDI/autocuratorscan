# Changes to compile on my ubuntu

* I had mpi but needed to install:
    * `conda install gcc_linux-64 gxx_linux-64 gfortran_linux-64 netcdf-cxx4 -c conda-forge`
* mpiCC hardcoded -> changed to mpic++ on my machine
    * mk/system/agri.make:11
* remove calls to GridObjects.h, VariableLookupObject.h,  RecapConfigObject.h
    * src/autocurator.cpp:20
* add include to autocurator
    * #include "netcdfcpp.h"
* correct -I flag
    * src/base/Makefile:13
        * CXXFLAGS+=-I$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/include
    * mk/defs.make:22
          * CXXFLAGS+= -I$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/include -I$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/
          * LDFLAGS+= -L$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/lib -L$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/
* autocurator line: `bin/autocurator --files "/1Tb/miniconda3/envs/nightly/share/cdat/sample_data/tas_ccsr-95a_1*nc" --out charles.csv`

