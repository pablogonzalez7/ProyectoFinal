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
    vehiculo.h vehiculo.cpp
    carretera.h carretera.cpp
    obstaculo.h obstaculo.cpp
    proyectil.h proyectil.cpp
    hacienda.h hacienda.cpp
    liana.h liana.cpp
    plataforma.h plataforma.cpp
    hipopotamo.h hipopotamo.cpp
    recursossprites.h recursossprites.cpp
    configuracionhacienda.h
    configuracioncarretera.h
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
        imagenes/hipopotamos.png
        imagenes/policias.png
        imagenes/carro_perry.png
        imagenes/carro_policia.png
        imagenes/bala.png
        imagenes/barrera.png
        imagenes/lodo.png
        imagenes/pisos_casa.png
        imagenes/muebles_casa.png
        imagenes/liana.png
        imagenes/agua_hacienda.png
        imagenes/suelo_hacienda.png
)

# Opcional: pruebas de mecanicas, rutas, reinicios y teclado.
option(CONSTRUIR_PRUEBAS "Compilar pruebas de casa y hacienda" OFF)
if(CONSTRUIR_PRUEBAS)
    enable_testing()
    foreach(prueba IN ITEMS casa lianas)
        qt_add_executable(pruebas_${prueba}
            pruebas/prueba_${prueba}.cpp
            mainwindow.h mainwindow.cpp mainwindow.ui
            personaje.h personaje.cpp
            jugador.h jugador.cpp
            villano.h villano.cpp
            nivel.h nivel.cpp
            llave.h llave.cpp
            vehiculo.h vehiculo.cpp
            carretera.h carretera.cpp
            obstaculo.h obstaculo.cpp
            proyectil.h proyectil.cpp
            hacienda.h hacienda.cpp
            liana.h liana.cpp
            plataforma.h plataforma.cpp
            hipopotamo.h hipopotamo.cpp
            recursossprites.h recursossprites.cpp
            configuracionhacienda.h
            configuracioncarretera.h
            configuracion.h
        )
        target_include_directories(pruebas_${prueba} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(pruebas_${prueba} PRIVATE Qt::Core Qt::Widgets)
        qt_add_resources(pruebas_${prueba} "imagenes_pruebas_${prueba}"
            PREFIX "/"
            FILES
                imagenes/perry.png
                imagenes/hipopotamos.png
                imagenes/policias.png
                imagenes/carro_perry.png
                imagenes/carro_policia.png
                imagenes/bala.png
                imagenes/barrera.png
                imagenes/lodo.png
                imagenes/pisos_casa.png
                imagenes/muebles_casa.png
                imagenes/liana.png
                imagenes/agua_hacienda.png
                imagenes/suelo_hacienda.png
        )
        add_test(NAME ${prueba} COMMAND pruebas_${prueba} ${CMAKE_CURRENT_BINARY_DIR}/capturas)
        set_tests_properties(${prueba} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
endif()
