project(MatchingEngine)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)
find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} Tests.cpp MatchingEngine.hpp)

target_link_libraries(
    ${PROJECT_NAME}
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    libgmock.a)
