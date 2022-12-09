cmake_minimum_required(VERSION 3.21.0)

project(
  sndrcv
  VERSION 0.1.0
  DESCRIPTION "SyncedGroups library using gstreamer")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_BUILD_TYPE Debug)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(
  GST
  REQUIRED
  gobject-2.0
  glib-2.0
  gstreamer-1.0
  gstreamer-app-1.0
  gstreamer-net-1.0)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} test/app.cpp src/receiver.cpp src/sender.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${GST_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${GST_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${GST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
