########################################################################
# Project setup -- only needed if device support is a stand-alone build
# We recommend that the support module be built in-tree with the driver.
########################################################################
cmake_minimum_required(VERSION 2.6)
project(SoapySDRMyDevice CXX)
enable_testing()
set(CMAKE_CXX_STANDARD 11)
#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

########################################################################
# Header and library resources needed to communicate with the device.
# These may be found within the build tree or in an external project.
########################################################################\
find_package(LibCyberRadio)

if ( NOT LIBCYBERRADIO_FOUND )
    message( FATAL_ERROR "LibCyberRadio Not found..." )
endif ()

set(LIBCYBERRADIO_SOAPY_INCLUDE_DIRS ${LIBCYBERRADIO_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBCYBERRADIO_SOAPY_LIBRARIES ${LIBCYBERRADIO_LIB})

message(STATUS "LIBCYBERRADIO_INCLUDE_DIRS - ${LIBCYBERRADIO_INCLUDE_DIR}")
message(STATUS "LIBCYBERRADIO_LIBRARIES - ${LIBCYBERRADIO_LIB}")

########################################################################
# build the module
########################################################################
find_package(SoapySDR CONFIG)

if (NOT SoapySDR_FOUND)
    message(WARNING "SoapySDR development files not found - skipping support")
    return()
endif ()

include_directories($LIBCYBERRADIO_SOAPY_INCLUDE_DIRS})
SOAPY_SDR_MODULE_UTIL(
    TARGET CyberRadioSoapy
    SOURCES CyberRadioSoapy.cpp
    LIBRARIES ${LIBCYBERRADIO_SOAPY_LIBRARIES}
)
