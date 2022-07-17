#include <ArduinoJson.h>
#include <EasyHTTP.h>
#include <WiFi.h>

// Librerias para el sensor DHT11
#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT11

// Detectamos el sensor
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastTime = 0;
unsigned long timerDelay = 10000; //Tiempo que tardará en ejecutarse el código en la función loop

//---------- Network info
char* ssid = "Adro312";   //Nombre de la red Ex. "upsrj_ALUMNOS_campus"
char* password = "4dRo312_E";

// Inicializas el objeto 'http' con los datos de la conexion a internet
EasyHTTP http(ssid, password);

// Guardas la url base de tu api aqui (con / al final)
String datosURL = "http://192.168.1.112/Oximetro-API/";

void setup() {
  Serial.begin(115200);

  // Inicializamos el sensor DHT11
  dht.begin();

  // Realizas la conexion a la api
  http.connectWiFi();
  http.setBaseURL(datosURL);
  Serial.println("Connected!");
}

void loop() {

  // Verificamos que el usuario haya cambiado el estatus para activar el oximetro
  String status_oximetro = http.get("API/getStatus");

  // Si el estado es 1, se ejecutara lo siguiente
  if (status_oximetro == "1") {

    // Declaramos las variables que vamos a enviar del sensor de temperatura y humedad
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    /* Para hacer un post
        Se debe de generar un JSON con los datos
    */
    // Inicializas la variable 'doc1' con 128kb de espacio (El default es 32 y te dará error si lo cambias pero te digo por whats como hacerlo xd)
    // De nuevo ten cuidado con el espacio en memoria xd
    DynamicJsonDocument doc1(128);
    // Inicializas una cadena llamada payload que tendra los datos que vas a mandar por POST
    String payload1 = "";

    /* Añades los datos que vas a mandar, en mi caso los datos del modelo usuario
      doc1["nombre_usuario"] = nombre;
      doc1["correo_usuario"] = correo;
    */
    doc1["humedad"] = h;
    doc1["temperatura"] = t;
    
    serializeJson(doc1, payload1); // Hace la conversión
    
    String response1 = http.post("API/saveData", payload1); // Url exacta y datos a mandar
    Serial.println(response1);
    Serial.println(payload1);
  }

  delay(5000);
}
