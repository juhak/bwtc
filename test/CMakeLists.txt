cmake_minimum_required(VERSION 2.6)

project(bwtc_tests)

set(EXECUTABLE_OUTPUT_PATH bin/)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system filesystem)

link_directories(${Boost_LIBRARY_DIR} ${OBJECT_FILE_PATH})
include_directories(${Boost_INCLUDE_DIR})

set(BOOST_UNIT_TESTS UtilsTest PrimeSequenceDetectorTest MaskSequenceDetectorTest
  WaveletTest)

foreach(program ${BOOST_UNIT_TESTS})
  add_executable(${program} ${program}.cpp)
  target_link_libraries(${program} common boost_unit_test_framework preprocessors)
  add_test(${program} ${EXECUTABLE_OUTPUT_PATH}/${program})
  set_tests_properties(${program} PROPERTIES FAIL_REGULAR_EXPRESSION 
    "[.\n]*failure")
endforeach(program)

add_executable(CodersTest CodersTest.cpp)
target_link_libraries(CodersTest common probmodels)
add_test(CodersTest ${EXECUTABLE_OUTPUT_PATH}/coders_test "test_file")
set_tests_properties(CodersTest PROPERTIES PASS_REGULAR_EXPRESSION ".*pass")

add_executable(SaisTest SaisTest.cpp)
target_link_libraries(SaisTest common)
add_test(SaisTest ${EXECUTABLE_OUTPUT_PATH}/sa-is_test)
set_tests_properties(SaisTest PROPERTIES PASS_REGULAR_EXPRESSION ".*pass")

add_executable(StreamTest StreamTest.cpp)
target_link_libraries(StreamTest common ${Boost_LIBRARIES})
add_test(StreamTest ${EXECUTABLE_OUTPUT_PATH}/stream_test "test_file")
set_tests_properties(StreamTest PROPERTIES PASS_REGULAR_EXPRESSION ".*pass")

add_executable(PreprocessorTest PreprocessorTest.cpp)
target_link_libraries(PreprocessorTest common preprocessors)
add_test(PreprocessorTest ${EXECUTABLE_OUTPUT_PATH}/preproc_test "test_file")
set_tests_properties(PreprocessorTest PROPERTIES PASS_REGULAR_EXPRESSION ".*pass")

add_executable(PreprocessorAlgoTest PreprocessorAlgoTest.cpp)
target_link_libraries(PreprocessorAlgoTest common preprocessors)

add_executable(BwtAndPreprocessorTest BwtAndPreprocessorTest.cpp)
target_link_libraries(BwtAndPreprocessorTest common preprocessors bwtransforms)

add_executable(LongSequencesTest LongSequencesTest.cpp)
target_link_libraries(LongSequencesTest common preprocessors)



#add_test(long_sequences_test ${EXECUTABLE_OUTPUT_PATH}/long_sequences_test)
#set_tests_properties(long_sequences_test PROPERTIES PASS_REGULAR_EXPRESSION ".*pass")