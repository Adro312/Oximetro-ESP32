#include <Wire.h>
#include "MAX30105.h" // LIBRERIA SENSOR DE OXIGENO
// #include <Adafruit_GFX.h>  // OLED libreria
#include <Adafruit_SSD1306.h>
#include "heartRate.h"

MAX30105 Sensor;

const byte RATE_SIZE = 4;   // incremento para obtener mas promedio
byte rates[RATE_SIZE];      // matriz de frecuencia cardiacas
byte rateSpot = 0;
long ultimolatido = 0;      // hora a la que ocurrio el ultimo latido
float latidosporminutos;
int mejorpromedio;

double oxi;

#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED en pixeles
#define SCREEN_HEIGHT 64 // Altura de la pantalla OLED en pixeles   
#define OLED_RESET -1    // Restablecer el pin # (o si -1 se comparte el pin de restablecimiento de arduino)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declaracion del nombre de la visualizacion

#define USEFIFO
void setup() 
{
    // Iniciamos la pantalla OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();

    // Puerto I2C predeterminado, velocidad de 400 kHz
    if ( !Sensor.begin(Wire, I2C_SPEED_FAST) ) { 
        Serial.println("Sensor no fue encontrado, Por favor checa que todo este bien conectado.");
        while (1);
    }

    // byte leadBrightness = 0x7F; // Opciones: 0 = Apagado a 255 = 50mA
    byte leadBrightness = 70; // Opciones: 0 = Apagado a 255 = 50mA
    byte sampleAverage = 4; // Opciones: 1, 2, 4, 8, 16, 32
    byte ledMode = 2; // OPciones: 1 = Solo rojo, 2 = Rojo + IR, 3 = Rojo + IR + Verde
    int sampleRate = 200; // Opciones: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411; // Opciones 69, 118, 215, 411
    int adcRange = 4096; // Opciones: 2048, 4096, 8192, 16384

    // Configurar los parametro deseados
    Sensor.setup(leadBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); // Configurar el sensor con estos ajustes
}

double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100; // Calcular SpO2 por este intervalo de muestreo

double ESpO2 = 93.0; // Valor inicial de la SpO2 estimada
double FSpO2 = 0.7; // Factor de filtro para la SpO2 estimada
double frate = 0.95; // Filtro de paso bajo para valor de LED IR / rojo para eliminar el componente de CA
#define TIMETOBOOT 2000 // Espere este tiempo (mseg) para generar SpO2
#define SCALE 88.0 // Ajustar para mostrar el latido del corazon y la SpO2 en la misma escala
#define SAMPLING 5 // Si desea ver los latidos del corazon con mayor precision, configure MUESTREO en 1
#define FINGER_ON 35000 // Si la señal roja es mas baja que esto, indica que su dedo no esta en el sensor
#define MINIMUM_SPO2 0.0

void loop() 
{
    uint32_t ir, red, green;
    double fred, fir;
    double SpO2 = 0; // SpO2 antes de filtrado de paso bajo

    #ifdef USEFIFO
        Sensor.check(); // Verifica el sensor, lea hasta 3 muestras
        while (Sensor.available()); // Tenemos nuevos datos
    #ifdef MAX30105
        red = Sensor.getFIFORed(); // Sparkfun's MAX30105
        ir = Sensor.getFIFOIR(); // Sparkfun's MAX30105
    #else
        red = Sensor.getFIFOIR(); // Why getFOFORed output IR data by MAX30102 on MH-ET LIVE breakout board
        ir = Sensor.getFIFORed(); // Why getFOFOIR output Red data by MAX30102 on MH-ET LIVE breakout board
    #endif
        i++;
        fred = (double)red;
        fir = (double)ir;
        avered = avered * frate + (double)red * (1.0 - frate); // Nivel promedio de rojo por filtro de paso bajo
        aveir = aveir * frate + (double)ir * (1.0 - frate); // Nivel de IR promedio por filtro de paso bajo
        sumredrms += (fred - avered) * (fred - avered); // Suma cuadrada del componente alternativo del nivel rojo
        sumirrms += (fir - aveir) * (fir - aveir); // Suma cuadrada del componente alternativo del nivel de IR

        // Ralentizar la velocidad de trazado del grafico para el trazador serial arduino por adelgazamiento
        if ( (i % SAMPLING) == 0 ) { 
            if ( millis() > TIMETOBOOT ) {
                if (ir < FINGER_ON) ESpO2 = MINIMUM_SPO2;
                if (ESpO2 <= -1) {
                    ESpO2 = 0;
                }
                if (ESpO2 > 100) {
                    ESpO2 = 100;
                }

                oxi = ESpO2;
                Serial.print("OXIGENO % = ");
                Serial.println(ESpO2);
            }
        }
        if ( (i % Num) == 0 ) {
            double R = (sqrt(sumredrms) / avered / (sqrt(sumirrms)) / aveir);
            SpO2 = -23.3 * (R - 0.4) + 100; // http://wwl.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
            ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2; // Filtro de paso bajo
            Serial.print("OXIGENO % = ");
            Serial.println(ESpO2);
            sumredrms = 0.0;
            sumirrms = 0.0;
            i = 0;
        }

        Sensor.nextSample(); // Hemos terminado con esta muestra, asi que pade a la siguiente 

        // ============================================================
        long irHR = Sensor.getIR();

        if (irHR > 7000) { // si se detecta un dedo
            display.clearDisplay(); // limpiar la pantalla
            display.setTextSize(1); // cerca de el muestra el promedio de BPM, puede mostrar el BPM si lo desea
            display.setTextColor(WHITE);
            display.setCursor(45, 2);
            display.setRotation(3);
            display.println("HR");
            display.setRotation(0);
            display.setTextSize(3);
            display.setCursor(20, 5);   
            display.println(mejorpromedio);
            display.setCursor(107, 5);
            display.setTextSize(1);
            display.print("bpm");    

            display.setTextSize(1); // Cerca de el muestra el BPM promedio, puede mostrar el BPM si lo desea
            display.setTextColor(WHITE);
            display.setRotation(3);
            display.setCursor(5, 2);
            display.println("SpO2");
            display.setTextSize(3);
            display.setRotation(0);
            display.setCursor(20, 5); 
            display.println(oxi, 1);
            display.setCursor(120, 35);
            display.setTextSize(1);
            display.print("%");
            display.display();
        }

        if ( irHR < 7000 ) { // Si no detecta un dedo
            mejorpromedio = 0; //reiniciar BPM promedio 
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(WHITE);
            display.setCursor(15,7);
            display.println("OxiCare");
            display.setCursor(30,35);
            display.setTextSize(3);
            display.println("by Deinev");

            display.display();
        }
    #endif
}