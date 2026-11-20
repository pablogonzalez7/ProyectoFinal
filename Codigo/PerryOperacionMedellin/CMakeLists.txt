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
    llave.h llave.cpp
    villano.h villano.cpp
    configuracion.h
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

# Opcional: pruebas de mecanicas, rutas, reinicios y teclado.
option(CONSTRUIR_PRUEBAS "Compilar las pruebas de la casa" OFF)
if(CONSTRUIR_PRUEBAS)
    enable_testing()
    qt_add_executable(pruebas_casa
        pruebas/prueba_casa.cpp
        mainwindow.h mainwindow.cpp mainwindow.ui
        personaje.h personaje.cpp
        jugador.h jugador.cpp
        villano.h villano.cpp
        nivel.h nivel.cpp
        llave.h llave.cpp
        configuracion.h
    )
    target_include_directories(pruebas_casa PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(pruebas_casa PRIVATE Qt::Core Qt::Widgets)
    qt_add_resources(pruebas_casa "imagenes_pruebas"
        PREFIX "/"
        FILES imagenes/perry.png
    )
    add_test(NAME casa COMMAND pruebas_casa ${CMAKE_CURRENT_BINARY_DIR}/capturas)
    set_tests_properties(casa PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
