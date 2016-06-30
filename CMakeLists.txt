cmake_minimum_required(VERSION 2.8)

project(Kobo-Aura-HD-Clean-Articles)

# Requirements
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(cmake_files
  "cmake/color_diagnostics.cmake"
)
foreach(file_ ${cmake_files})
  include("${file_}")
endforeach()

add_executable(Kobo-Aura-HD-Clean-Articles main.cpp deletedir.h Debug.h)
