cmake_minimum_required(VERSION 3.28)
project(EquiStabilize)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_executable(EquiStabilize
    main.cpp
    src/engine/OrderBook.cpp
)
