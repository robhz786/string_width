
add_executable(generate_ch32_width_gcb_prop_tree generate_ch32_width_gcb_prop_tree.cpp)

target_link_libraries(
  generate_ch32_width_gcb_prop_tree
  icuuc
  icudata
  strf::strf-header-only
)

