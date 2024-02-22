//Librería necesaria para el sensor de temperatura y humedad del ambiente
#include <DHT.h> //Biblioteca necesaria para utilizar el sensor DHT22
#include <WiFi.h> //Biblioteca necesaria para utilizar el Wifi de la placa ESP32
#include <Firebase_ESP_Client.h>//Biblioteca necesaria para utilizar la base de datos Firebase de Google y escritura y lectura del ESP32
#include <addons/TokenHelper.h>//Biblioteca que nos inidica los errores tipo addons
#include <addons/RTDBHelper.h>

int currentDataCount;//Contador actual del número de datos leido por los sensores

//Nombre y contraseña para WiFi
const char* ssid = "NOMBRE DEL WIFI";
const char* password = "CONTRASEÑA DEL WIFI";

//Variables para la base de datos
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//Variables que son auxiliares para la conexión del ESP32 y la escritura en Firebase
unsigned long sendDataPrevMillis = 0;
bool signupok = false;

//Constantes del programa
#define tiemposubida 1500
#define API_KEY "LLAVE DE LA BASE DE DATOS"
#define DATABASE_URL "URL DE LA BASE DE DATOS"

//Pines del ESP32 para cada uno de los sensores y sus conexiones
#define DHTPIN 4
#define DHTTYPE DHT22
#define ledRojo 21
#define ledVerde 19
#define ledAmarillo 25
#define valorplanta 50
#define maxData 100
int distancia = 0;
int pinEcho = 2;
int pinTrigg = 15;
const int humsuelo1 = 35;
int valhumsuelo;
DHT dht(DHTPIN, DHTTYPE); //Creación de un objeto tipo DHT para el sensor de temperatura DHT22 o DHT11

//Función que recibe los pines del sensor ultrasonico y retorna el tiempo que se tardo en regresar el pulso electromagnetico.
long readUltrasonicDistance(int triggerPin, int echoPin)
{
  //Iniciamos el pin del emisor de reuido en salida
  pinMode(triggerPin, OUTPUT);
  //Apagamos el emisor de sonido
  digitalWrite(triggerPin, LOW);
  //Retrasamos la emision de sonido por 2 milesismas de segundo
  delayMicroseconds(2);
  // Comenzamos a emitir sonido
  digitalWrite(triggerPin, HIGH);
  //Retrasamos la emision de sonido por 2 milesismas de segundo
  delayMicroseconds(10);
  //Apagamos el emisor de sonido
  digitalWrite(triggerPin, LOW);
  //Comenzamos a escuchar el sonido
  pinMode(echoPin, INPUT);
  // Calculamos el tiempo que tardo en regresar el sonido
  return pulseIn(echoPin, HIGH);
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a internet y base de datos\n");
  }
  Serial.print("Conected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth,"","")){
    Serial.println("OK");
    signupok = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(humsuelo1, INPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarillo, OUTPUT);
  dht.begin();
}

void loop() {
////////////////////////Aquí empieza lo de humedad de suelo///////////////////////
  valhumsuelo = map(analogRead(humsuelo1),4095,0,0,100);

  if(valhumsuelo < valorplanta){
    digitalWrite(ledRojo, HIGH);
    digitalWrite(ledVerde, LOW);
  }
  else{
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledVerde, HIGH);
  }
/////////////////////Aquí termina humedad de suelo//////////////////////////

////////////////////Aquí comienza lo de temperatura y humedad ambiental/////////////////////////////////
  // Leemos la humedad relativa  
  float h = dht.readHumidity();
  // Leemos la temperatura en grados centígrados (por defecto)
  float t = dht.readTemperature();
  // Leemos la temperatura en grados Fahreheit
  float f = dht.readTemperature(true);

  // Comprobamos si ha habido algún error en la lectura
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }
  // Calcular el índice de calor en Fahreheit
  float hif = dht.computeHeatIndex(f, h);
  // Calcular el índice de calor en grados centígrados
  float hic = dht.computeHeatIndex(t, h, false);
  ///////////////////////////////////Aquí termina lo de temperatura y humedad ambiental/////////////////////

  ////////////////////////////////////Aquí empieza lo de distancia/////////////////////////////////
  distancia = 0.01723 * readUltrasonicDistance(pinTrigg, pinEcho);
  if (distancia > 5){
    digitalWrite(ledAmarillo, HIGH);
  }
  else{
    digitalWrite(ledAmarillo, LOW);
  }
  /////////////////////////////////////////Aquí termina lo de distancia///////////////////////////////////
  if(Firebase.ready() && signupok && (millis() -sendDataPrevMillis > tiemposubida || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    currentDataCount++;
    // Utilizar setInt con el contador actualizado como identificador
    if (Firebase.RTDB.setInt(&fbdo, "Valores/Humedad_de_suelo/" + String(currentDataCount), valhumsuelo)) {
      Serial.println("PASSED");
      Serial.println("PATH: Valores/Humedad_de_suelo/" + String(currentDataCount));
      Serial.println("TYPE: " + fbdo.dataType());
    }
    if (Firebase.RTDB.setInt(&fbdo, "Valores/Humedad_ambiental/" + String(currentDataCount), h)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }

    if (Firebase.RTDB.setInt(&fbdo, "Valores/Temperatura/" + String(currentDataCount), t)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }

    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Índice de calor: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");
    Serial.print("Humedad del suelo: ");
    Serial.print(valhumsuelo);
    Serial.println("%");
    if (distancia != 0){
      if (Firebase.RTDB.setInt(&fbdo, "Valores/Distancia/" +  String(currentDataCount), distancia)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      }
      Serial.print("La distancia es: ");
      Serial.println(distancia);
    }
    if (currentDataCount >49){
      currentDataCount = 0;
    }
    Firebase.RTDB.setInt(&fbdo, "Valores/ContadorActualcount", currentDataCount);
  }
}