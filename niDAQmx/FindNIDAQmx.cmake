# #########################################################################
# #########################################################################
# CMake FIND file for the proprietary NIDAQmx Windows library (NIDAQmx).
#
# Try to find NIDAQmx
# Once done this will define
# NIDAQMX_FOUND - System has NIDAQmx
# NIDAQMX_LIBRARY - The NIDAQmx library
# NIDAQMX_INCLUDE_DIR - The NIDAQmx include file
# #########################################################################
# #########################################################################
# #########################################################################
# Useful variables

if(WIN32)
    if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
      list(APPEND NIDAQMX_DIR 
	    "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev" 
	    "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C")
      set(SUFFIXES "lib/msvc"
		    "lib32/msvc")
    else()
      list(APPEND NIDAQMX_DIR 
	    "C:/Program Files/National Instruments/NI-DAQ/DAQmx ANSI C Dev" 
	    "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C")
      set(SUFFIXES "lib/msvc"
		    "lib64/msvc")
    endif()
else()
    set(NIDAQMX_DIR "/usr/include" "/usr/lib/x86_64-linux-gnu")
    set(SUFFIXES "")
endif()

# Find installed library using CMake functions
find_library(NIDAQMX_LIBRARY
	NAMES "NIDAQmx" "nidaqmx"
	PATHS ${NIDAQMX_DIR}
	PATH_SUFFIXES ${SUFFIXES})

find_path(NIDAQMX_INCLUDE_DIR
	NAMES "NIDAQmx.h"
	PATHS ${NIDAQMX_DIR}
	PATH_SUFFIXES "include")

# Handle the QUIETLY and REQUIRED arguments and set NIDAQMX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NIDAQmx DEFAULT_MSG NIDAQMX_LIBRARY NIDAQMX_INCLUDE_DIR)
# #########################################################################

# #########################################################################
mark_as_advanced(NIDAQMX_LIBRARY NIDAQMX_INCLUDE_DIR)
# #########################################################################