// Librearias para poder conectarse con la API
#include <ArduinoJson.h>
#include <EasyHTTP.h>
#include <WiFi.h>

// Claves para conectarse al internet
char* ssid = "Adro312";   //Nombre de la red Ex. "upsrj_ALUMNOS_campus"
char* password = "4dRo312_E";

// Inicializas el objeto 'http' con los datos de la conexion a internet
EasyHTTP http(ssid, password);

// Se guarda la URL de la API
String API_URL = "http://192.168.1.112/Oximetro-API/";

void setup() {
  Serial.begin(115200);


  /**
   * 
   * 
   * Colar codigo del nuevo sensor
   * 
   * 
   * 
  */


  // Realizas la conexion a la api
  if ( http.connectWiFi() ) {
    Serial.println("Connected!");
    http.post("API/saveData", "1");
  } else {
    Serial.println("Internet don't found");
  }

  http.setBaseURL(API_URL);
}

void loop() {

  // Verificamos que el usuario haya cambiado el estatus para activar el oximetro
  String status_oximetro = http.get("API/getStatus");

  // Si el estado es 1, se ejecutara lo siguiente
  if (status_oximetro == "1") {

    // Declaramos las variables que vamos a enviar del sensor
    float hr = 0;
    float oxi = 0;
    float temp = 0;


    /**
     * 
     * 
     * Colar codigo del nuevo sensor
     * 
     * 
     * 
    */


    // Se nicializa la variable 'doc1' con 128kb de espacio (El default es 32 y te dará error si lo cambias)
    // De nuevo ten cuidado con el espacio en memoria xd
    DynamicJsonDocument doc1(128);

    // Inicializas una cadena llamada payload que tendra los datos que vas a mandar por POST
    String payload1 = "";

    doc1["oxigenacion"] = oxi;
    doc1["ritmo_cardiaco"] = hr;
    doc1["temperatura"] = temp;
    
    serializeJson(doc1, payload1); // Hace la conversión
    
    String response1 = http.post("API/saveData", payload1); // Url exacta y datos a mandar
    Serial.println(response1);
    Serial.println(payload1);
  }

  delay(3000);
}
