
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ../)

add_executable(BufferTest BufferTest.cc)
target_link_libraries(BufferTest DiamondCommon ${GTEST_BOTH_LIBRARIES} pthread)

add_test(BUFFERTEST BufferTest)