
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ../)

add_executable(BufferTest BufferTest.cc)
target_link_libraries(BufferTest diamond_common ${GTEST_BOTH_LIBRARIES} pthread)

add_executable(rioCache rioCache.cc)
target_link_libraries(rioCache diamond_common diamond_rio ${GTEST_BOTH_LIBRARIES} pthread)

add_executable(map128 map128.cc)
target_link_libraries(map128 diamond_common ${GTEST_BOTH_LIBRARIES} pthread)

add_executable(kv kv.cc)
target_link_libraries(kv diamond_common ${GTEST_BOTH_LIBRARIES} pthread)

add_test(BUFFERTEST BufferTest)
add_test(RIOTEST rioCache)
add_test(KV kv)
