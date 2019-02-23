cmake_minimum_required(VERSION 3.1.1 FATAL_ERROR)

# Git config
find_package(Git)
if(EXISTS "${CMAKE_SOURCE_DIR}/.git" AND Git_FOUND)
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" rev-list --count --topo-order v${VERSION_FULL}..HEAD
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
endif()

if(WIN32)
	# Resource VersionInfo
	set(PROJECT_PRODUCT_NAME "Peon Library")
	set(PROJECT_COMPANY_NAME "Rodrigo Holztrattner Reis")
endif()

set(PROJECT_NAME "Peon")
set(PEON_FILES "Peon")

project(${PROJECT_NAME} VERSION 1.0.0 DESCRIPTION "blob")

add_library(${PROJECT_NAME} STATIC
Peon/PeonJob.cpp
Peon/PeonMemoryAllocator.cpp
Peon/PeonStealingQueue.cpp
Peon/PeonSystem.cpp
Peon/PeonWorker.cpp
)

# set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER Peon/Peon.h)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT library)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)    