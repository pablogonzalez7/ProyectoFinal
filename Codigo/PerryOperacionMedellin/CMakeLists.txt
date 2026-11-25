cmake_minimum_required(VERSION 3.19)
project(PerryOperacionMedellin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

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
    guardiaentrada.h guardiaentrada.cpp
    entradahacienda.h entradahacienda.cpp
    arenapopeye.h arenapopeye.cpp
    popeye.h popeye.cpp
    proyectilcombate.h proyectilcombate.cpp
    desenlace.h desenlace.cpp
    escobar.h escobar.cpp
    audiojuego.h audiojuego.cpp
    dificultad.h eventossonido.h
    memoriacombate.h
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
        Qt::Multimedia
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
        imagenes/asfalto_carretera.png
        imagenes/llano_carretera.png
        imagenes/panorama_hacienda.png
        imagenes/popeye.png
        imagenes/interior_hacienda.png
        imagenes/escobar.png
        sonidos/perry.mp3
        sonidos/popeye.mp3
        sonidos/victoria_popeye.mp3
        sonidos/accion.wav
        sonidos/alarma.wav
        sonidos/camuflaje.wav
        sonidos/derrota.wav
        sonidos/disparo.wav
        sonidos/escudo.wav
        sonidos/impacto.wav
        sonidos/llave.wav
        sonidos/salto.wav
        sonidos/sigilo.wav
        sonidos/victoria.wav
)

# Opcional: pruebas de mecanicas, rutas, reinicios y teclado.
option(CONSTRUIR_PRUEBAS "Compilar pruebas de casa y hacienda" OFF)
if(CONSTRUIR_PRUEBAS)
    enable_testing()
    foreach(prueba IN ITEMS casa lianas margenes entrada popeye desenlace audio_dificultad musica)
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
            guardiaentrada.h guardiaentrada.cpp
            entradahacienda.h entradahacienda.cpp
            arenapopeye.h arenapopeye.cpp
            popeye.h popeye.cpp
            proyectilcombate.h proyectilcombate.cpp
            desenlace.h desenlace.cpp
            escobar.h escobar.cpp
            audiojuego.h audiojuego.cpp
            dificultad.h eventossonido.h
            memoriacombate.h
            liana.h liana.cpp
            plataforma.h plataforma.cpp
            hipopotamo.h hipopotamo.cpp
            recursossprites.h recursossprites.cpp
            configuracionhacienda.h
            configuracioncarretera.h
            configuracion.h
        )
        target_include_directories(pruebas_${prueba} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(pruebas_${prueba} PRIVATE Qt::Core Qt::Widgets Qt::Multimedia)
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
                imagenes/asfalto_carretera.png
                imagenes/llano_carretera.png
                imagenes/panorama_hacienda.png
                imagenes/popeye.png
                imagenes/interior_hacienda.png
                imagenes/escobar.png
                sonidos/perry.mp3
                sonidos/popeye.mp3
                sonidos/victoria_popeye.mp3
                sonidos/accion.wav
                sonidos/alarma.wav
                sonidos/camuflaje.wav
                sonidos/derrota.wav
                sonidos/disparo.wav
                sonidos/escudo.wav
                sonidos/impacto.wav
                sonidos/llave.wav
                sonidos/salto.wav
                sonidos/sigilo.wav
                sonidos/victoria.wav
        )
        add_test(NAME ${prueba} COMMAND pruebas_${prueba} ${CMAKE_CURRENT_BINARY_DIR}/capturas)
        set_tests_properties(${prueba} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
endif()
