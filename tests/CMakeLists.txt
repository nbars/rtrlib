add_executable(test_pfx test_pfx.c)
target_link_libraries(test_pfx rtrlib)
add_executable(test_lpfst test_lpfst.c)
target_link_libraries(test_lpfst rtrlib)
add_executable(test_pfx_locks test_pfx_locks.c)
target_link_libraries(test_pfx_locks rtrlib)
add_executable(test_ht_spkitable test_ht_spkitable.c)
target_link_libraries(test_ht_spkitable rtrlib)
add_executable(test_ht_spkitable_locks test_ht_spkitable_locks.c)
target_link_libraries(test_ht_spkitable_locks rtrlib)

add_executable(test_live_validation test_live_validation.c)
target_link_libraries(test_live_validation rtrlib)

#TODO: Make conditional
#make test CTEST_OUTPUT_ON_FAILURE=TRUE
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    message(STATUS "Valgrind found! Some tests will now run with valgrind enabled.")
    set(valgrind_cmd ${VALGRIND_EXECUTABLE} --track-origins=yes --leak-check=full --error-exitcode=255)
else(VALGRIND_EXECUTABLE)
    message(STATUS "Could not find valgrind, tests will run without valgrind support!")
endif(VALGRIND_EXECUTABLE)

#pfx table tests
ADD_TEST(test_pfx test_pfx)
ADD_TEST(test_lpfst test_lpfst)
#ADD_TEST(test_pfx_locks tests/test_pfx_locks)

#spki table tests
ADD_TEST(test_ht_spkitable test_ht_spkitable)
ADD_TEST(test_ht_spkitable_locks test_ht_spkitable_locks)

#Live validation test
ADD_TEST(test_live_validation ./test_live_validation)

ADD_SUBDIRECTORY(test_network)
