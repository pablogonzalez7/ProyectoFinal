"""Genera WAV originales con sintesis. Solo se usa al preparar los recursos.
El juego C++ no necesita Python ni bibliotecas de audio externas a Qt.
"""
from pathlib import Path
import math
import random
import wave
import sys
from array import array

DESTINO = Path(__file__).resolve().parent.parent / 'sonidos'
MUESTREO = 48000
rng = random.Random(731)

def guardar(nombre, muestras):
    pico = max(abs(x) for x in muestras) or 1
    # Dos canales identicos: conserva el sonido centrado y evita salida mono.
    datos = array('h')
    for x in muestras:
        valor = round(x / pico * 0.72 * 32767)
        datos.extend((valor, valor))
    if sys.byteorder != 'little':
        datos.byteswap()
    with wave.open(str(DESTINO / (nombre + '.wav')), 'wb') as salida:
        salida.setnchannels(2)
        salida.setsampwidth(2)
        salida.setframerate(MUESTREO)
        salida.writeframes(datos.tobytes())

def nota(numero):
    return 440 * 2 ** ((numero - 69) / 12)

def agregar(pista, inicio, duracion, frecuencia, amplitud, timbre='suave'):
    primero = int(inicio * MUESTREO)
    for n in range(int(duracion * MUESTREO)):
        indice = primero + n
        if indice >= len(pista):
            break
        t = n / MUESTREO
        envolvente = min(1, t / .008) * max(0, 1 - t / duracion) ** 1.8
        fase = 2 * math.pi * frecuencia * t
        valor = math.sin(fase) + .18 * math.sin(2 * fase) + .08 * math.sin(3 * fase)
        if timbre == 'bajo':
            valor = math.sin(fase) + .11 * math.sin(3 * fase)
        pista[indice] += amplitud * envolvente * valor

def musica(nombre, bpm):
    pulso = 60 / bpm
    pista = [0.0] * round(32 * pulso * MUESTREO)
    raices = [45, 41, 50, 40, 45, 41, 50, 40]
    motivos = [[0,7,10,3,7,0,12,10], [3,7,0,10,7,12,3,0],
               [0,3,7,12,10,7,3,10], [7,0,4,11,7,4,2,0]]
    for compas, raiz in enumerate(raices):
        for tiempo in range(4):
            inicio = (4 * compas + tiempo) * pulso
            agregar(pista, inicio, pulso * .8, nota(raiz), .35, 'bajo')
            for n in range(int(.12 * MUESTREO)):
                i = int(inicio * MUESTREO) + n
                t = n / MUESTREO
                pista[i] += .18 * math.exp(-30*t) * math.sin(2*math.pi*(80*t-90*t*t))
        for medio, intervalo in enumerate(motivos[compas % 4]):
            inicio = (4 * compas + medio / 2) * pulso
            agregar(pista, inicio, pulso * .42, nota(raiz + 24 + intervalo), .16)
            for n in range(int(.035 * MUESTREO)):
                i = int((4*compas+(medio+.5)/2)*pulso*MUESTREO) + n
                if i < len(pista):
                    pista[i] += .035 * rng.uniform(-1,1) * math.exp(-n/(150*MUESTREO/22050))
    borde = int(.025 * MUESTREO)
    for i in range(borde):
        pista[i] *= i / borde
        pista[-1-i] *= i / borde
    guardar(nombre, pista)

def barrido(nombre, inicio, fin, duracion, ruido=0):
    muestras = []
    for n in range(round(duracion * MUESTREO)):
        t = n / MUESTREO
        fase = 2*math.pi*(inicio*t+(fin-inicio)*t*t/(2*duracion))
        amplitud = min(1,t/.005) * (1-t/duracion)**1.6
        muestras.append(amplitud*(math.sin(fase)+ruido*rng.uniform(-1,1)))
    guardar(nombre,muestras)

def acorde(nombre, notas, intervalo):
    pista = [0.0] * round((len(notas)*intervalo+.14)*MUESTREO)
    for i, tono in enumerate(notas):
        agregar(pista,i*intervalo,intervalo+.13,nota(tono),.45)
    guardar(nombre,pista)

if __name__ == '__main__':
    DESTINO.mkdir(exist_ok=True)
    musica('sigilo',100)
    musica('accion',130)
    acorde('llave',[76,83,88],.09)
    acorde('alarma',[81,76,81,76],.16)
    barrido('disparo',850,140,.13,.16)
    barrido('impacto',150,50,.16,.55)
    barrido('escudo',300,1400,.35)
    acorde('victoria',[69,73,76,81],.16)
    acorde('derrota',[69,65,60,57],.18)
    barrido('salto',260,700,.15)
    barrido('camuflaje',1200,280,.32,.06)
