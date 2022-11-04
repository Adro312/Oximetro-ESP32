// Librearias para poder conectarse con la API
#include <ArduinoJson.h>
#include <EasyHTTP.h>
#include <WiFi.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <Adafruit_SSD1306.h>

// Size de la pantalla
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1

// Definimos el sensor y la pantalla
MAX30105 particleSensor;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Cosas necesarias del sensor
#define MAX_BRIGHTNESS 255

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

byte pulseLED = 11; //Must be on PWM pin
byte readLED = 13; //Blinks with each data read

// Claves para conectarse al internet
//char* ssid = "upsrj_ALUMNOS_campus";
//char* password = "";
char* ssid = "Adro312";   
char* password = "4dRo312_E";

// Inicializas el objeto 'http' con los datos de la conexion a internet
EasyHTTP http(ssid, password);

// Se guarda la URL de la API
String API_URL = "http://192.168.1.111/Oximetro-API/";

void setup() {
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("Sensor no fue encontrado, Por favor checa que todo este bien conectado.");
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,2);
    display.println("No Sensor");

    display.display();
    while(1);
  }

  // Mostramos en pantalla que todo esta bien
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,2);
  display.println("OxiCare");
  display.display();

  // Realizas la conexion a la api
  http.connectWiFi();
  
  // cambiar status de la coneccion de internet
  http.setBaseURL(API_URL);

  http.get("API/changeStatusOxi");
}

void loop() {
  // Seguimos manteniendo el estatus del oximetro que sigue conectado
  http.get("API/changeStatusOxi"); 

  // Mostramos imagen normal antes de empezar
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,2);
  display.println("OxiCare");
  display.display();

  // Verificamos que el usuario haya cambiado el estatus para activar el oximetro
  String status_oximetro = http.get("API/getStatusApp");

  // Si el estado es 1, se ejecutara lo siguiente
  if (status_oximetro == "1") {
    pinMode(pulseLED, OUTPUT);
    pinMode(readLED, OUTPUT);

    // Calibracion del sensor
    byte ledBrightness = 60; //Options: 0=Off to 255=50mA
    byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
    byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411; //Options: 69, 118, 215, 411
    int adcRange = 4096; //Options: 2048, 4096, 8192, 16384
  
    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

    // Declaramos las variables que vamos a enviar del sensor
    float hr = 0;
    float oxi = 0;
    float temp = 0;

    // Mostramos en pantalla de que ya va a iniciar
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Manten tu dedo");
    display.println("en el sensor rojo.");
    display.println("No lo quites");
    display.display();
    delay(2000);

    // EMPEZAMOS CON EL RITMO CARDIACO Y EL OXIGENO
    bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

    //read the first 100 samples, and determine the signal range
    for (byte i = 0 ; i < bufferLength ; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.println(irBuffer[i], DEC);
      delay(100);
    }

    //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    bool band = true;

    while(band) { 
      //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  
      //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
      for (byte i = 25; i < 100; i++)
      {
        redBuffer[i - 25] = redBuffer[i];
        irBuffer[i - 25] = irBuffer[i];
      }


      //take 25 sets of samples before calculating the heart rate.
      for (byte i = 75; i < 100; i++)
      {
        while (particleSensor.available() == false) //do we have new data?
          particleSensor.check(); //Check the sensor for new data
  
        digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read
  
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample(); //We're finished with this sample so move to next sample
  
        display.clearDisplay();
  
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.print("HR: ");
        display.print(heartRate);
        display.println("bpm");
        display.print("Spo2: ");
        display.print(spo2);
        display.print("%");
        
        display.display();
  
        hr = heartRate;
        oxi = spo2;
      }

      //After gathering 25 new samples recalculate HR and SP02
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

      if (hr != -999 && oxi != -999 && hr > 60 && hr < 170 && oxi > 85 ) {
          delay(1000);
          band = false;
      }
    }

    // EMPEZAMOS LA TEMPERATURA
    //The LEDs are very low power and won't affect the temp reading much but
    //you may want to turn off the LEDs to avoid any local heating
    particleSensor.setup(0); //Configure sensor. Turn off LEDs
    //particleSensor.setup(); //Configure sensor. Use 25mA for LED drive

    particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.

    float temperature = particleSensor.readTemperature();

    int rest = 5;

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.print("Temp:");
    display.println(temperature + rest);
    display.display();

    temp = temperature + rest;

    delay(1000);

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Enviando");
    display.println("Datos...");
    display.display();

    // Se nicializa la variable 'doc1' con 128kb de espacio (El default es 32 y te dará error si lo cambias)
    // De nuevo ten cuidado con el espacio en memoria xd
    DynamicJsonDocument doc1(128);

    // Inicializas una cadena llamada payload que tendra los datos que vas a mandar por POST
    String payload1 = "";

    doc1["oxygen"] = oxi;
    doc1["heart_rate"] = hr;
    doc1["temperature"] = temp;
    
    serializeJson(doc1, payload1); // Hace la conversión
    
    String response1 = http.post("API/saveData", payload1); // Url exacta y datos a mandar
    Serial.println(response1);
    Serial.println(payload1);

    delay(3000);

    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Listo!");
    display.display();
  }

  delay(3000);
}
