cmake_minimum_required(VERSION 3.5)
project(LSPClientExample LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 14)

# Qt packages
find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)

# Define your executable
add_executable(LSPClientExample
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
)

# Include headers
target_include_directories(LSPClientExample PUBLIC
    ../include             # Your own headers
    ../third_party         # Any third-party headers
    C:/Users/Casa/Desktop/lsp-cpp-master/include  # LSPClient headers
)

# Link .lib location (directory containing LSPClient.lib)
link_directories("C:/Users/Casa/Desktop/lsp-cpp-master/build/Desktop_Qt_6_9_0_MSVC2022_64bit-Debug")

# Link with Qt and your LSPClient static lib
target_link_libraries(LSPClientExample
    Qt6::Core
    Qt6::Widgets
    "C:/Users/Casa/Desktop/lsp-cpp-master/build/Desktop_Qt_6_9_0_MSVC2022_64bit-Debug/LSPClient.lib"
)
