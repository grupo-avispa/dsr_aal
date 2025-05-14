cmake_minimum_required(VERSION 3.5)
project(webserver_agent)

# Default to C++20
if(NOT CMAKE_CXX_STANDARD)
  if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 20)
  else()
    message(FATAL_ERROR "cxx_std_20 could not be found.")
  endif()
endif()

# ########################
# # Find  Dependencies  ##
# ########################
find_package(oatpp 1.3.0 REQUIRED)
find_package(oatpp-websocket 1.3.0 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL)
find_package(fastrtps REQUIRED)

# Set include directories
include_directories(
  ${CMAKE_SOURCE_DIR}/components/webserver_agent/include
)

# Set QT libraries DSR libraries and OATPP libraries
set(QT_LIBRARIES Qt5::Widgets Qt5::OpenGL Qt5::Core)
set(DSR_LIBRARIES dsr_api dsr_core dsr_gui fastcdr fastrtps)
set(OATPP_LIBRARIES oatpp::oatpp oatpp::oatpp-websocket)

# Set the headers
set(headers
  ${CMAKE_SOURCE_DIR}/components/webserver_agent/include/DSR_interface.hpp
)

# Qt Moc
qt5_wrap_cpp(qt_moc ${headers}
  OPTIONS --no-notes # Don't display a note for the headers which don't produce a moc_*.cpp
)

# ########################
# # User Libraries      ##
# ########################
add_library(web_dsr src/DSR_interface.cpp)
target_link_libraries(web_dsr
  PUBLIC ${OATPP_LIBRARIES} ${DSR_LIBRARIES}
  PRIVATE Eigen3::Eigen ${QT_LIBRARIES}
)
target_sources(web_dsr PRIVATE ${qt_moc})

add_library(websocket src/WSListener.cpp)
target_link_libraries(websocket
  PUBLIC ${OATPP_LIBRARIES} ${DSR_LIBRARIES}
  PRIVATE web_dsr ${QT_LIBRARIES}
)

# ########################
# # Main Program        ##
# ########################
add_executable(webserver_agent src/webserver_agent.cpp)
target_link_libraries(webserver_agent
  PUBLIC ${OATPP_LIBRARIES} ${DSR_LIBRARIES}
  PRIVATE web_dsr websocket Eigen3::Eigen ${QT_LIBRARIES}
)

# ########################
# # Install             ##
# ########################

# Install in the devel path
# INSTALL(TARGETS web_dsr websocket
# DESTINATION ${COMPONENT_DEVEL_PATH}/lib
# )

# INSTALL(TARGETS webserver_agent
# DESTINATION ${COMPONENT_DEVEL_PATH}/bin
# )

# Install in the install path (/opt/campero)
INSTALL(TARGETS web_dsr websocket
  DESTINATION ${COMPONENT_INSTALL_PATH}/lib
)

install(TARGETS webserver_agent
  DESTINATION ${COMPONENT_INSTALL_PATH}/bin
)

INSTALL(FILES etc/config
  DESTINATION ${COMPONENT_INSTALL_PATH}/etc-default/
  RENAME ${PROJECT_NAME}.conf
)