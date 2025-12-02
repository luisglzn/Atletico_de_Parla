# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-src"
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-build"
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix"
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix/tmp"
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp"
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix/src"
  "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/c/Users/matan/Desktop/Oesia/Robocup/Player/build/_deps/min_sock-subbuild/min_sock-populate-prefix/src/min_sock-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
