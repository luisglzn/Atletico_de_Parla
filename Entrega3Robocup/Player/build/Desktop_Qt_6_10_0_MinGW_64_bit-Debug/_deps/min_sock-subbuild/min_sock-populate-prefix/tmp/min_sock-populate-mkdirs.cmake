# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-src")
  file(MAKE_DIRECTORY "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-build"
  "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix"
  "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix/tmp"
  "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp"
  "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix/src"
  "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/matan/Desktop/Oesia/Formacion/Player/build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
