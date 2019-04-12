# Copyright (c) 2016      Bryce Adelstein-Lelbach aka wash
# Copyright (c) 2000-2016 Paul Ullrich 
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Base directory.
HYPERIONCLIMATEDIR=..

# Load Makefile framework. 
include $(HYPERIONCLIMATEDIR)/mk/framework.make

ifeq ($(NETCDF), TRUE)
  BUILD_TARGETS= netcdf-cxx-4.2
  NETCDFCXX42LIB= $(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/libnetcdfcxx4.2.a
  HYPERIONCLIMATELIBS+= $(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2/libnetcdf_c++.a
  HYPERIONCLIMATELDFLAGS+= -L$(HYPERIONCLIMATEDIR)/src/netcdf-cxx-4.2
endif

BUILD_TARGETS+= base contrib
CLEAN_TARGETS= $(addsuffix .clean,$(BUILD_TARGETS))

HYPERIONCLIMATELIBS+= $(HYPERIONCLIMATEDIR)/src/base/libhyperionbase.a

HYPERIONCLIMATELDFLAGS+= -L$(HYPERIONCLIMATEDIR)/src/base -L$(HYPERIONCLIMATEDIR)/src/contrib

LIBRARIES+= -lhyperionbase -lhyperioncontrib

EXEC_FILES= autocurator.cpp

EXEC_TARGETS= $(EXEC_FILES:%.cpp=%)

FILES= $(UTIL_FILES) $(EXEC_FILES)

.PHONY: all clean $(BUILD_TARGETS)

# Build rules. 
all: $(BUILD_TARGETS) $(EXEC_TARGETS)

$(BUILD_TARGETS): %:
	cd $*; $(MAKE)

$(EXEC_TARGETS): %: $(BUILD_TARGETS) $(BUILDDIR)/%.o $(HYPERIONCLIMATELIBS)
	$(CXX) $(LDFLAGS) $(HYPERIONCLIMATELDFLAGS) -o $@ $(UTIL_FILES:%.cpp=$(BUILDDIR)/%.o) $(BUILDDIR)/$*.o $(LIBRARIES)
	mv $@ $(HYPERIONCLIMATEDIR)/bin

# Clean rules.
clean: $(CLEAN_TARGETS)
	rm -rf $(DEPDIR)
	rm -rf $(BUILDDIR)

$(CLEAN_TARGETS): %.clean:
	cd $*; $(MAKE) clean

# Include dependencies.
-include $(FILES:%.cpp=$(DEPDIR)/%.d)

# DO NOT DELETE
