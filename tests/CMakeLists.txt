include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(basic_tests basic_tests.cpp)
add_test(NAME basic_tests COMMAND basic_tests)

add_executable(ch32_width_gcb_prop_tree ch32_width_gcb_prop_tree.cpp)
target_link_libraries(ch32_width_gcb_prop_tree icuuc icudata strf::strf-header-only)
add_test(NAME ch32_width_gcb_prop_tree COMMAND ch32_width_gcb_prop_tree)

