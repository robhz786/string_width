
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_executable(basic_tests basic_tests.cpp)
add_test(NAME basic_tests COMMAND basic_tests)
