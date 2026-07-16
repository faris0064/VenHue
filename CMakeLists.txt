cmake_minimum_required(VERSION 3.16)

project(VenHue VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    add_compile_options(-Wno-deprecated -Wno-dynamic-exception-spec)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Quick Widgets)

set(VENHUE_EDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK" CACHE PATH
    "Path to the Philips Hue EDK source")
set(VENHUE_EDK_BUILD_DIR "${VENHUE_EDK_ROOT}/build" CACHE PATH
    "Path to the Philips Hue EDK build")

set(_venhue_edk_build_command
    "cmake -S \"${VENHUE_EDK_ROOT}\" -B \"${VENHUE_EDK_BUILD_DIR}\" -DBUILD_TEST=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TOOLS=OFF -DBUILD_SWIG=OFF -DBUILD_WRAPPERS=OFF, then cmake --build \"${VENHUE_EDK_BUILD_DIR}\" --config Release")

if(NOT EXISTS "${VENHUE_EDK_ROOT}/libhuestream/huestream/HueStream.h")
    message(FATAL_ERROR
        "Philips Hue EDK not found at ${VENHUE_EDK_ROOT}. "
        "Pull EDK repo there or set VENHUE_EDK_ROOT.")
endif()

set(_venhue_edk_library_dirs
    "${VENHUE_EDK_BUILD_DIR}/bin"
    "${VENHUE_EDK_BUILD_DIR}/external_install/lib"
    "${VENHUE_EDK_BUILD_DIR}/external_install/lib64")

function(venhue_find_edk_library variable)
    find_library(${variable}
        NAMES ${ARGN}
        PATHS ${_venhue_edk_library_dirs}
        NO_DEFAULT_PATH)
    if(NOT ${variable})
        list(JOIN ARGN ", " _names)
        message(FATAL_ERROR
            "Philips Hue EDK (${_names}) not found under ${VENHUE_EDK_BUILD_DIR}. "
            "Build it first with ${_venhue_edk_build_command}")
    endif()
    set(${variable} "${${variable}}" PARENT_SCOPE)
endfunction()

venhue_find_edk_library(VENHUE_EDK_HUESTREAM huestream)
venhue_find_edk_library(VENHUE_EDK_BRIDGE_DISCOVERY bridge_discovery)
venhue_find_edk_library(VENHUE_EDK_SUPPORT support)
venhue_find_edk_library(VENHUE_EDK_EDTLS_CLIENT edtls_client)
venhue_find_edk_library(VENHUE_EDK_JSON json)
venhue_find_edk_library(VENHUE_EDK_CURL curl libcurl)
venhue_find_edk_library(VENHUE_EDK_MBEDCL_WRAPPER mbedcl_wrapper)
venhue_find_edk_library(VENHUE_EDK_MBEDCRYPTO mbedcrypto)
venhue_find_edk_library(VENHUE_EDK_MBEDTLS mbedtls)
venhue_find_edk_library(VENHUE_EDK_MBEDX509 mbedx509)
venhue_find_edk_library(VENHUE_EDK_MDNS_RESPONDER mdns_responder)
venhue_find_edk_library(VENHUE_EDK_NGHTTP2 nghttp2_static)

add_library(VenHueEDK INTERFACE IMPORTED)
target_include_directories(VenHueEDK INTERFACE
    "${VENHUE_EDK_ROOT}/libhuestream"
    "${VENHUE_EDK_ROOT}/libhuestream/bridgediscovery/include"
    "${VENHUE_EDK_ROOT}/libhuestream/support/include"
    "${VENHUE_EDK_BUILD_DIR}/external_install/include")

target_link_libraries(VenHueEDK INTERFACE
    "${VENHUE_EDK_HUESTREAM}"
    "${VENHUE_EDK_BRIDGE_DISCOVERY}"
    "${VENHUE_EDK_SUPPORT}"
    "${VENHUE_EDK_EDTLS_CLIENT}"
    "${VENHUE_EDK_JSON}"
    "${VENHUE_EDK_CURL}"
    "${VENHUE_EDK_MBEDCL_WRAPPER}"
    "${VENHUE_EDK_MBEDTLS}"
    "${VENHUE_EDK_MBEDX509}"
    "${VENHUE_EDK_MBEDCRYPTO}"
    "${VENHUE_EDK_MDNS_RESPONDER}"
    "${VENHUE_EDK_NGHTTP2}")

if(WIN32)
    target_compile_definitions(VenHueEDK INTERFACE NOMINMAX)
    target_link_libraries(VenHueEDK INTERFACE
        ws2_32 iphlpapi netapi32 bcrypt crypt32 winmm wldap32 normaliz)
else()
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Brotli REQUIRED IMPORTED_TARGET libbrotlidec)
    pkg_check_modules(Zstd REQUIRED IMPORTED_TARGET libzstd)
    target_link_libraries(VenHueEDK INTERFACE
        Threads::Threads
        ${CMAKE_DL_LIBS}
        m
        PkgConfig::Brotli
        PkgConfig::Zstd)
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appVenHue
    src/main.cpp
)

qt_add_qml_module(appVenHue
    URI VenHue
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        src/animated_light_effect.cpp
        src/controller.cpp
        src/hue_connection_state.cpp
        src/light_effect_loader.cpp
        src/light_director.cpp
        src/venue_monitor.cpp
        src/configuration_manager.cpp
        src/logger.cpp
        src/parser.cpp
        include/animated_effect_definition.h
        include/animated_light_effect.h
        include/controller.h
        include/hue_connection_state.h
        include/light_effect_loader.h
        include/light_director.h
        include/venue_monitor.h
        include/configuration_manager.h
        include/logger.h
        include/parser.h
)

set_target_properties(appVenHue PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appVenHue PRIVATE include)
target_link_libraries(appVenHue PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Quick
    Qt6::Widgets
    VenHueEDK
)

add_custom_target(copy_light_effects ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config/light_effects.json"
        "$<TARGET_FILE_DIR:appVenHue>/light_effects.json"
    DEPENDS
        appVenHue
        "${CMAKE_CURRENT_SOURCE_DIR}/config/light_effects.json"
    VERBATIM)

include(GNUInstallDirs)
install(TARGETS appVenHue
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES config/light_effects.json DESTINATION ${CMAKE_INSTALL_BINDIR})
