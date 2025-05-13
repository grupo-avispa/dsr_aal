cmake_minimum_required(VERSION 3.5)
project(speechAgent)

# Default to C++20
if(NOT CMAKE_CXX_STANDARD)
  if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 20)
  else()
    message(FATAL_ERROR "cxx_std_20 could not be found.")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wdeprecated -fPIC -Wshadow -Wnull-dereference)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>")
endif()

find_package(Eigen3 3.3 REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL)
find_package(fastrtps REQUIRED)

# Set include directories
include_directories(
  include
  ${QT_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
  ${fastrtps_INCLUDE_DIR}
)

# Set QT libraries and DSR libraries
set(QT_LIBRARIES Qt5::Widgets Qt5::OpenGL Qt5::Core)
set(DSR_LIBRARIES dsr_api dsr_core dsr_gui fastcdr fastrtps)

# Set the names
set(library_name ${PROJECT_NAME}_core)
set(executable_name ${PROJECT_NAME})

# Set  dependencies
set(dependencies
  ${QT_LIBRARIES}
  ${DSR_LIBRARIES}
  Eigen3::Eigen
  spdlog
  fmt
)

# Set the headers
set(headers
  include/${PROJECT_NAME}/speech_agent.hpp
)

set(sources
  src/speech_agent.cpp
)

# Qt Moc
qt5_wrap_cpp(qt_moc ${headers}
  OPTIONS --no-notes # Don't display a note for the headers which don't produce a moc_*.cpp
)

# Add libraries
add_library(speech_manager SHARED src/sound_manager.cpp src/speech_dispatcher.cpp)

add_library(${library_name} SHARED ${sources})
target_link_libraries(${library_name} ${dependencies} speech_manager)
target_sources(${library_name} PRIVATE ${qt_moc})

# Add executble
add_executable(${executable_name} src/main.cpp)
target_link_libraries(${executable_name} ${library_name})

# Install in the devel path
# INSTALL(TARGETS ${library_name}
# DESTINATION ${COMPONENT_DEVEL_PATH}/lib
# )

# INSTALL(TARGETS ${executable_name}
# DESTINATION ${COMPONENT_DEVEL_PATH}/bin
# )

# Install in the install path (/opt/campero)
INSTALL(TARGETS ${library_name} speech_manager
  DESTINATION ${COMPONENT_INSTALL_PATH}/lib
)

install(TARGETS ${executable_name}
  DESTINATION ${COMPONENT_INSTALL_PATH}/bin
)

INSTALL(FILES etc/config
  DESTINATION ${COMPONENT_INSTALL_PATH}/etc-default/
  RENAME ${PROJECT_NAME}.conf
)
