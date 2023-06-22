cmake_minimum_required(VERSION 3.14)

# Setup vcpkg script with CMake (note: should be placed before project() call)
list(APPEND VCPKG_FEATURE_FLAGS "versions")
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

set(CMAKE_CXX_STANDARD 17)

project(app LANGUAGES CXX)

add_executable(app src/main.cpp)

find_package(OpenGL REQUIRED)
target_link_libraries(app PRIVATE OpenGL::GL)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(app PRIVATE glfw)

find_package(GLEW REQUIRED)
target_link_libraries(app PRIVATE GLEW::GLEW)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(app PRIVATE imgui::imgui)

find_package(SqliteOrm CONFIG REQUIRED)
target_link_libraries(app PRIVATE sqlite_orm::sqlite_orm)