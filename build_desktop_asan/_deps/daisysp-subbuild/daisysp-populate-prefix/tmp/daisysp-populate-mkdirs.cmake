# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-src"
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-build"
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix"
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix/tmp"
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix/src/daisysp-populate-stamp"
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix/src"
  "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix/src/daisysp-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix/src/daisysp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ashokfernandez/Software/PolySynth/build_desktop_asan/_deps/daisysp-subbuild/daisysp-populate-prefix/src/daisysp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
