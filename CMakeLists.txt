CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(ClienServerEcn)

FIND_PACKAGE(Boost 1.40 COMPONENTS system REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_COMPILE_OPTIONS(-Wall -Werror -Wextra -Wpedantic)

# Enable code coverage
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  ADD_COMPILE_OPTIONS(--coverage)
  ADD_LINK_OPTIONS(--coverage)
endif()

ADD_EXECUTABLE(Server Server.cpp Core.cpp Common.hpp json.hpp)
TARGET_LINK_LIBRARIES(Server PRIVATE Threads::Threads ${Boost_LIBRARIES})

ADD_EXECUTABLE(Client Client.cpp Common.hpp json.hpp)
TARGET_LINK_LIBRARIES(Client PRIVATE Threads::Threads ${Boost_LIBRARIES})

ADD_EXECUTABLE(Test Core.cpp tests/CoreTest.cpp)
TARGET_LINK_LIBRARIES(Test PRIVATE gtest gtest_main)

# Coverage target
ADD_CUSTOM_TARGET(coverage
    COMMAND ${CMAKE_COMMAND} -E env GCOV_PREFIX=${CMAKE_BINARY_DIR}
    gcovr -r .. --html --html-details -o coverage/coverage.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS Test
)
