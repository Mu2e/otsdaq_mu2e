# ======================================================================
#  otsdaq-mu2e main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/otsdaq-mu2e/ups/setup_for_development
#  buildtool
# ======================================================================


# use cmake 3.17 or later
cmake_minimum_required (VERSION 3.17)


project(otsdaq-mu2e)
# cetbuildtools contains our cmake modules
SET ( CETBUILDTOOLS_VERSION $ENV{CETBUILDTOOLS_VERSION} )
IF (NOT CETBUILDTOOLS_VERSION)
    MESSAGE (FATAL_ERROR "ERROR: setup cetbuildtools to get the cmake modules")
ENDIF()

set(CMAKE_MODULE_PATH
        $ENV{ART_DIR}/Modules
        $ENV{CETBUILDTOOLS_DIR}/Modules
        $ENV{XDAQ_DIR}/Modules
        $ENV{ARTDAQ_UTILITIES_DIR}/Modules
        ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")

include(CetCMakeEnv)
cet_cmake_env()
include(FindUpsXdaq)

cet_set_compiler_flags(DIAGS VIGILANT
 WERROR
 NO_UNDEFINED
 EXTRA_FLAGS -pedantic -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-ignored-qualifiers -Wno-unused-variable
 )

#string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
#find_ups_product( epics v3_15_2 )
find_ups_product( cetbuildtools v3_03_00 )
find_ups_product( artdaq v0_04_02 )
find_ups_product( artdaq_core v1_00_00 )
find_ups_product( artdaq_utilities v1_00_00 )
find_ups_product( art v1_06_00 )
find_ups_product( fhiclcpp v2_16_00 )
find_ups_product( messagefacility v1_10_00 )
find_ups_product( cetlib v1_03_00 )
find_ups_product( otsdaq v1_00_00 )
find_ups_product( otsdaq_utilities v1_00_00 )
find_ups_product( otsdaq_components v1_00_00 )
find_ups_product( pcie_linux_kernel_module v1_11_01 )
find_ups_product( TRACE v3_03_00 )
find_ups_xdaq( v11_2_7 )
find_ups_root( v5_34_00 )
find_ups_boost( v1_50_0 filesystem
                        program_options
                        unit_test_framework
 )


find_package(MPI QUIET)

# macros for art_dictionary and simple_plugin

include(ArtMake)
include(BuildPlugins)
include(CetTest)

# Code
add_subdirectory(otsdaq-mu2e)

# Useful scripts
add_subdirectory(tools)

# Test Programs
#add_subdirectory(test)

# Documentation directory
if( $ENV{OTS_DOXY} MATCHES "DOIT" )
	add_subdirectory(doc)
endif()

# UPS
add_subdirectory(ups)


# packaging utility
include(UseCPack)
