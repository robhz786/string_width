cmake_minimum_required (VERSION 3.11)
project(string_width LANGUAGES CXX)
enable_testing()

option(STRING_WIDTH_BUILD_TESTS "Build unit tests" ON)
option(STRING_WIDTH_BUILD_TOOLS "Build programs in tools/" ON)

if (STRING_WIDTH_BUILD_TOOLS)
  # must run 'git submodule update --init'
  add_subdirectory(external/strf strf)

endif (STRING_WIDTH_BUILD_TOOLS)

if (STRING_WIDTH_BUILD_TESTS OR STRING_WIDTH_BUILD_TOOLS)

  # ICU libraries must be manually installed external/icu
  set(icu_dir ${CMAKE_CURRENT_SOURCE_DIR}/external/icu)
  set(icu_lib_dir ${icu_dir}/lib)
  set(icu_include_dir ${icu_dir}/include)

  add_library(icudata SHARED IMPORTED)
  add_library(icuuc   SHARED IMPORTED)

  set_property(TARGET icudata PROPERTY IMPORTED_LOCATION ${icu_lib_dir}/libicudata.so)
  set_property(TARGET icuuc   PROPERTY IMPORTED_LOCATION ${icu_lib_dir}/libicuuc.so)

  target_include_directories(icudata INTERFACE ${icu_include_dir})
  target_include_directories(icuuc   INTERFACE ${icu_include_dir})

endif (STRING_WIDTH_BUILD_TESTS OR STRING_WIDTH_BUILD_TOOLS)

if (STRING_WIDTH_BUILD_TESTS)
  add_subdirectory(tests)
endif (STRING_WIDTH_BUILD_TESTS)

if (STRING_WIDTH_BUILD_TOOLS)
  add_subdirectory(tools)
endif (STRING_WIDTH_BUILD_TOOLS)

