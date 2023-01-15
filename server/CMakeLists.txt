cmake_minimum_required(VERSION 3.10)
project(myproject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread")

set(SOURCES
    client.cpp
    game.cpp 
    handler.cpp 
    main.cpp 
    connection.cpp 
    timer.cpp
    cardImport.cpp
)

add_executable(main ${SOURCES})

target_link_libraries(main jsoncpp)

add_custom_target(cppcheck
    COMMAND cppcheck --enable=all --std=c++17 -I include/ ${SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck")


# add cppcheck as a dependency of main target
add_dependencies(main cppcheck)