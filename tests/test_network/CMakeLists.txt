add_executable(example_01 example_01.c)
target_link_libraries(example_01 rtr_tfwlib)

add_executable(pdu_wrong_length pdu_wrong_length.c)
target_link_libraries(pdu_wrong_length rtr_tfwlib)

#Network tests
ADD_TEST(example_01 ./example_01)
ADD_TEST(pdu_wrong_length ./pdu_wrong_length)
