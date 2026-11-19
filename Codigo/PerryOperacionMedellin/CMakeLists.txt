cmake_minimum_required(VERSION 3.19)
project(PerryOperacionMedellin LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(PerryOperacionMedellin
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    personaje.h personaje.cpp
    jugador.h jugador.cpp
    nivel.h nivel.cpp
)

target_link_libraries(PerryOperacionMedellin
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS PerryOperacionMedellin
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET PerryOperacionMedellin
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

qt_add_resources(PerryOperacionMedellin "imagenes_juego"
    PREFIX "/"
    FILES
        imagenes/perry.png
)
