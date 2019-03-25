cmake_minimum_required (VERSION 2.6)
project (multper)
# The version number.
set (multper_VERSION_MAJOR 1)
set (multper_VERSION_MINOR 0)

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE not specified.  Defaulting to 'Release'")
  message(STATUS "Usage: cmake -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel] .")
  set(CMAKE_BUILD_TYPE Release)
else()
  message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

if(MPLIB STREQUAL "CPP")
    message(STATUS "Building for cpp_int")
    add_definitions(-DCPP_INT)
    # no external linking needed
elseif(MPLIB STREQUAL "TOM")
    message(STATUS "Building for tom_int")
    add_definitions(-DTOM_INT)
    set(MP_LIBRARY -ltommath)
elseif(MPLIB STREQUAL "GMP")
    message(STATUS "Building for mpz_int (gmp)")
    add_definitions(-DMPZ_INT)
    set(MP_LIBRARY -lgmp)
else()
    message(FATAL_ERROR "Please specify a multiprecision library with \"cmake -DMPLIB=name\"\nLibrary name options are: \"GMP\", \"CPP\", or \"TOM\"")
endif()

# add the executable
add_executable(multper multiplicative_persistence.cc)
target_link_libraries (multper ${MP_LIBRARY})