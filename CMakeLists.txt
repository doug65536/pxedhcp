#/*
# * This file is part of PXEDHCP.
# * Copyright 2013 A. Douglas Gale
# *
# * PXEDHCP is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * PXEDHCP is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# */

cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE "Debug")

project(pxedhcp)
set(project_name ${CMAKE_PROJECT_NAME})

find_package(Qt4 REQUIRED QtCore QtNetwork)

#set(CMAKE_AUTOMOC TRUE)

include(${QT_USE_FILE})

add_definitions(${QT_DEFINITIONS})

include_directories(
 ${CMAKE_CURRENT_BINARY_DIR}
 ${QT_INCLUDE_DIRS}
)

set(${project_name}_readme_names
    README.txt
    LICENSE.txt
)

file(GLOB ${project_name}_sources *.cpp)
file(GLOB ${project_name}_headers *.h)
file(GLOB ${project_name}_forms *.ui)
file(GLOB ${project_name}_resources *.qrc)
file(GLOB ${project_name}_readme ${${project_name}_readme_names})

set(${project_name}_sources_list
    ${${project_name}_sources}
    ${${project_name}_headers}
    ${${project_name}_forms}
    ${${project_name}_resources}
)

qt4_wrap_cpp(${project_name}_headers_moc ${${project_name}_headers})
qt4_wrap_ui(${project_name}_forms_headers ${${project_name}_forms})
qt4_add_resources(${project_name}_resources_rcc ${${project_name}_resources})

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W3 -Wextra -pedantic")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
endif()

add_definitions(-DQT_DEBUG=1)

add_executable(${project_name}
    ${${project_name}_sources_list}
    ${${project_name}_headers_moc}
    ${${project_name}_forms_headers}
    ${${project_name}_resources_rcc}
    ${${project_name}_readme}
)
target_link_libraries(${project_name} ${QT_LIBRARIES})

install(TARGETS ${project_name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tiny boot server")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "bin")
set(CPACK_PACKAGE_EXECUTABLES "${project_name}")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Doug Gale")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt4-core, libqt4-network")

include(CPack)
