set(CMAKE_BUILD_TYPE Debug)

add_executable(fuzz_rtr_sync fuzz_rtr_sync.c)
target_link_libraries(fuzz_rtr_sync rtrlib_static)
