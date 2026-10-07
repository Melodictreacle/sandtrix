cmake_minimum_required(VERSION 3.16)

project(sandtrix VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

set(SOURCES
    src/main.cpp
    src/Tetromino.cpp
    src/SandEngine.cpp
    src/ParticleSystem.cpp
    src/AudioManager.cpp
    src/UIPanels.cpp
    src/GameCanvas.cpp
    src/MainWindow.cpp
)

set(HEADERS
    src/Config.h
    src/Tetromino.h
    src/SandEngine.h
    src/ParticleSystem.h
    src/AudioManager.h
    src/UIPanels.h
    src/GameCanvas.h
    src/MainWindow.h
)

add_executable(sandtrix
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(sandtrix PRIVATE src)

target_link_libraries(sandtrix PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
)

if(WIN32)
    set_target_properties(sandtrix PROPERTIES
        WIN32_EXECUTABLE FALSE # Set to TRUE if no console window is desired
    )
endif()
