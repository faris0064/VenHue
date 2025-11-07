# I don't think this even compiles correctly on linux anymore
cmake_minimum_required(VERSION 3.16)

project(VenHue VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    add_compile_options(-Wno-deprecated)
    add_compile_options(-Wno-dynamic-exception-spec)
    add_compile_definitions(NOMINMAX)
endif()

find_package(Qt6 REQUIRED COMPONENTS Quick Widgets)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")
file(GLOB RESOURCES "resources/*")

include_directories(
    "include"
    "libs/EDK-2.2.0/libhuestream"                          
    "libs/EDK-2.2.0/libhuestream/bridgediscovery/include"  
    "libs/EDK-2.2.0/libhuestream/support/include"          
    "libs/EDK-2.2.0/build/external_install/include"        
)

# Phillips Hue EDK libraries
set(EDK_LIBRARIES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/bin/huestream.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/bin/bridge_discovery.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/bin/support.lib"
)

# libraries required by the Hue EDK
set(EDK_EXTERNAL_LIBRARIES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/edtls_client.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/everest.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/json.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/libcurl.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/mbedcl_wrapper.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/mbedcrypto.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/mbedtls.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/mbedx509.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/mdns_responder.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/nghttp2_static.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/EDK-2.2.0/build/external_install/lib/p256m.lib"
)

if(WIN32)
    set(SYSTEM_LIBRARIES
        ws2_32       
        iphlpapi     
        netapi32     
        bcrypt       
        crypt32      
        winmm
        wldap32
        normaliz
    )
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
        src/controller.cpp
        src/light_director.cpp
        src/venue_monitor.cpp
        src/configuration_manager.cpp
        src/logger.cpp
        src/parser.cpp
        include/controller.h
        include/light_director.h
        include/venue_monitor.h
        include/configuration_manager.h
        include/logger.h
        include/parser.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appVenHue PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appVenHue
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appVenHue
    PRIVATE 
    Qt6::Quick
    Qt6::Widgets
    ${EDK_LIBRARIES}
    ${EDK_EXTERNAL_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)

include(GNUInstallDirs)
install(TARGETS appVenHue
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
