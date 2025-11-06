#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Configuración del hardware
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4  // Número de módulos 8x8 conectados

// Pines del ESP8266
#define DATA_PIN 13   // D7 → DIN
#define CLK_PIN 14    // D5 → CLK
#define CS_PIN 15     // D8 → CS

// Inicializar el objeto Parola
MD_Parola matriz = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Estructura para animaciones
struct animacion {
  textEffect_t anim_in;
  textEffect_t anim_out;
  const char *textOut;
  uint16_t speed;
  uint16_t pause;
  textPosition_t just;
};

// Lista de mensajes con diferentes animaciones
animacion lista[] = {
  // Autos → desplazamiento lateral
  { PA_SCROLL_DOWN, PA_SCROLL_DOWN , "Autos", 5, 2, PA_CENTER },

  //Zonas -> Autos
  { PA_SCROLL_LEFT, PA_SCROLL_UP , "5  plazas ", 5, 3, PA_CENTER },

  // Motos → desplazamiento diagonal hacia abajo a la izquierda
  { PA_SCROLL_DOWN, PA_SCROLL_DOWN , "Motos", 5, 2, PA_CENTER },

  // Zonas -> Motos
  { PA_SCROLL_LEFT, PA_SCROLL_UP , "20  plazas ", 5, 3, PA_CENTER },

  // Camiones → desplazamiento de abajo hacia arriba
  { PA_SCROLL_DOWN, PA_SCROLL_DOWN , "Camion", 5, 2, PA_CENTER },

    // Zonas -> Camiones
  { PA_SCROLL_LEFT, PA_SCROLL_UP , "1   plaza ", 5, 3, PA_CENTER },
};

void setup() {
  matriz.begin();
  matriz.setIntensity(6);   // Brillo de 0 (mínimo) a 15 (máximo)
  matriz.displayClear();

  // Ajustar velocidad y pausas
  for (uint8_t i = 0; i < ARRAY_SIZE(lista); i++) {
    lista[i].speed *= matriz.getSpeed();  // Velocidad relativa
    lista[i].pause *= 1000;               // Pausa entre animaciones
  }
}

void loop() {
  static uint8_t i = 0;

  if (matriz.displayAnimate()) {  // Cuando termina una animación
    if (i == ARRAY_SIZE(lista)) i = 0;  // Reinicia la secuencia
    matriz.displayText(lista[i].textOut, lista[i].just, lista[i].speed,
                       lista[i].pause, lista[i].anim_in, lista[i].anim_out);
    delay(1000);  // Pequeña pausa antes del siguiente mensaje
    i++;
  }
}

