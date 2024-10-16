# ======================================================================
#  otsdaq-mu2e main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/otsdaq-mu2e/ups/setup_for_development
#  buildtool
# ======================================================================

cmake_minimum_required (VERSION 3.19 FATAL_ERROR)

find_package(cetmodules 3.19.02 REQUIRED)

project(otsdaq_mu2e VERSION 3.03.00)

include(CetCMakeEnv)
cet_cmake_env()
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

cet_set_compiler_flags(DIAGS VIGILANT
 WERROR
 NO_UNDEFINED
 EXTRA_FLAGS -pedantic -Wno-unused-parameter
 )

#string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

#cet_report_compiler_flags()

find_package(otsdaq 2.06.08 REQUIRED EXPORT) 
find_package(otsdaq_components 2.06.08 REQUIRED EXPORT)
find_package(artdaq_mu2e 1.05.02 REQUIRED EXPORT)
find_package(PostgreSQL 13.2 REQUIRED)

 # XDAQ Extra setup
 include_directories($ENV{XDAQ_ROOT}/include/linux $ENV{XDAQ_ROOT}/include)
link_directories($ENV{XDAQ_ROOT}/lib)
set(XDAQ_BASIC_LIB_LIST 
   xcept
   xoap
   xdaq
   toolbox
   xerces-c
   cgicc
 )
 set(XDAQ_XERCES-C xerces-c)
 

# macros for art_dictionary and simple_plugin

include(BuildPlugins)

# Code
add_subdirectory(otsdaq-mu2e)

# test programs
add_subdirectory(test)

# Useful script tools
add_subdirectory(tools)
FILE(COPY tools DESTINATION .)
INSTALL(DIRECTORY tools DESTINATION . FILE_PERMISSIONS USE_SOURCE_PERMISSIONS)

# Test Programs
#add_subdirectory(test)
#FILE(COPY test DESTINATION .)
#INSTALL(DIRECTORY test DESTINATION ${product}/${version})

# Documentation (to comment out if not needed DOXYGEN locally)
if( $ENV{OTS_DOXY} MATCHES "DOIT" )
	add_subdirectory(doc)
endif()

cet_cmake_config()
