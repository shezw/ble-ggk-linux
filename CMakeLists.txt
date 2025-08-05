
cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

project(ble-ggk-linux C CXX)

if (NOT PROJ_DIR)
    set(PROJ_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

include(${PROJ_DIR}/options.cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)

if (NOT GLIB_FOUND)
    message(FATAL_ERROR "GLib not found. Please install the glib development package.")
else()
    message(STATUS "Found GLib: ${GLIB_VERSION}")
    message(STATUS "GLib include directories: ${GLIB_INCLUDE_DIRS}")
    message(STATUS "GLib library directories: ${GLIB_LIBRARY_DIRS}")
    message(STATUS "GLib library flags: ${GLIB_LDFLAGS}")
    message(STATUS "GLib libraries: ${GLIB_LIBRARIES}")
endif ()


file(GLOB PROJ_PRIVATE_SRCS ${PROJ_DIR}/src/*.cpp)

file(GLOB PROJ_INCLUDE_FILES ${PROJ_DIR}/src/*.h ${PROJ_DIR}/include/*.h)

include_directories(${PROJ_DIR}/src ${PROJ_DIR}/include)

list(REMOVE_ITEM PROJ_PRIVATE_SRCS ${PROJ_DIR}/src/standalone.cpp)

add_library(${PROJECT_NAME} STATIC ${PROJ_PRIVATE_SRCS})

add_executable(${PROJECT_NAME}_standalone ${PROJ_DIR}/src/standalone.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${GLIB_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${GLIB_LIBRARY_DIRS})

target_include_directories(${PROJECT_NAME}_standalone PUBLIC ${GLIB_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME}_standalone PUBLIC ${GLIB_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    glib-2.0 gobject-2.0 gio-2.0 gmodule-2.0
    pthread 
    pcre ffi z
)

target_link_libraries(${PROJECT_NAME}_standalone
    ${PROJECT_NAME}
    ${GLIB_LIBRARIES}
    pthread
)

configure_file("${PROJ_DIR}/lib${PROJECT_NAME}.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(TARGETS ${PROJECT_NAME}_standalone DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJ_INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ble-ggk-linux)
