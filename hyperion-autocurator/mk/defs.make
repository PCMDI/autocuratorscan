# Copyright (c) 2016      Bryce Adelstein-Lelbach aka wash
# Copyright (c) 2000-2016 Paul Ullrich 
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Set compiler flags and preprocessor defines.

###############################################################################
# Configuration-independent configuration.

CXXFLAGS+= -std=c++11

ifndef HYPERIONCLIMATEDIR
  $(error HYPERIONCLIMATEDIR is not defined)
endif

# Add the source directories to the include path
CXXFLAGS+= -I$(HYPERIONCLIMATEDIR)/src/base

ifeq ($(NETCDF),TRUE)
  CXXFLAGS+= -I$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/
  LDFLAGS+= -L$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/
endif

###############################################################################
# Configuration-dependent configuration.

ifeq ($(OPT),TRUE)
  # NDEBUG disables assertions, among other things.
  CXXFLAGS+= -O3 -DNDEBUG 
  F90FLAGS+= -O3
else
  CXXFLAGS+= -O0
  F90FLAGS+= -O0 
endif

ifeq ($(DEBUG),TRUE)
  # Frame pointers give us more meaningful stack traces in OPT+DEBUG builds.
  CXXFLAGS+= -ggdb -fno-omit-frame-pointer
  F90FLAGS+= -g
endif

ifeq ($(PARALLEL),MPIOMP)
  CXXFLAGS+= -DHYPERION_MPIOMP 
  CXX= $(MPICXX)
  F90= $(MPIF90)
else ifeq ($(PARALLEL),NONE)
else
  $(error mk/config.make does not properly define PARALLEL)
endif

ifeq ($(NETCDF),TRUE)
  CXXFLAGS+=  -DHYPERION_NETCDF $(NETCDF_CXXFLAGS)
  LIBRARIES+= $(NETCDF_LIBRARIES)
  LDFLAGS+=   $(NETCDF_LDFLAGS)
endif

# DO NOT DELETE
