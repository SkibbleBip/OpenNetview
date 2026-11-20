
cmake_minimum_required(VERSION 3.16)

project(OpenNetview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale
)

add_executable(opennetviewclient
    main.cpp
    TextBox.cpp
    WebConnect.cpp
    VideoManager.cpp
)

target_link_libraries(opennetviewclient PRIVATE
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
    PkgConfig::FFMPEG
)

target_include_directories(opennetviewclient PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/inc
	${CMAKE_CURRENT_SOURCE_DIR}/../inc
)
