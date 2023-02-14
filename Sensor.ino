

// ========================================================================================================
// --- Bibliotecas Auxiliares ---
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include "WiFiEsp.h"

// ========================================================================================================
// --- Mapeamento de Hardware ---
#define    ONE_WIRE_BUS     4          //sinal do sensor DS18B20

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
  SoftwareSerial Serial1(6,7); // RX, TX 
#endif
#define ID_MQTT  "Teste 01" //id mqtt definido como as coordenada geográficas para garantir unicidade

// ========================================================================================================
// --- Declaração de Objetos ---
OneWire oneWire(ONE_WIRE_BUS);        //objeto one_wire
DallasTemperature sensors(&oneWire);
DeviceAddress sensor1;

WiFiEspClient espClient; // Cria o objeto espClient

PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

// MQTT
const char* BROKER_MQTT = "192.168.1.110"; // IP/URL DO BROKER MQTT
int BROKER_PORT = 1883; // Porta do Broker MQTT

// ========================================================================================================
// --- Protótipo das Funções ---
void mostra_endereco_sensor(DeviceAddress deviceAddress);  //Função para mostrar endereço do sensor
void printCurrentNet();
void printWifiData();
void imprimi_temperatura(); 
void reconnectMQTT();
void recconectWiFi();

// ========================================================================================================
// --- Variáveis Globais ---
float tempMin = 999;   //armazena temperatura mínima
float tempMax = 0;     //armazena temperatura máxima
float tempC;
int TTT=0;

//Casa
char ssid[] = "NAME";            // your network SSID (name)
char pass[] = "PASSWORD";        // your network password


int status = WL_IDLE_STATUS;     // the Wifi radio's status



// ========================================================================================================
// --- Configurações Iniciais ---
void setup() 
{
  //Saida na tela
  Serial.begin(115200);
  Serial.println("Inicio do programa");

  
  // initializa serial para ESP module
  Serial1.begin(9600);

  //Inicialeza sensor
  sensors.begin();

  // initialize ESP module
  Serial.println("Inicio WiFi.init");
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("You're connected to the network");

  Serial.println("Init MQTT");
  initMQTT();
  
  // Localiza e mostra enderecos dos sensores
  Serial.println("Localizando sensores DS18B20...");
  Serial.print("Foram encontrados ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" sensores.");
  
  if (!sensors.getAddress(sensor1, 0)) {
     Serial.println("Sensores nao encontrados !"); 
     while (true); 
  }
  // Mostra o endereco do sensor encontrado no barramento
  Serial.print("Endereco sensor: ");
  mostra_endereco_sensor(sensor1);
  Serial.println();
  Serial.println(); 

} //end setup

// ========================================================================================================
// --- Loop Infinito ---
void loop() 
{
  Serial.println();
  
  imprimi_temperatura();
  
  printCurrentNet();
  printWifiData();
  Serial.println();
  recconectWiFi();
  if (!MQTT.connected()) {
    reconnectMQTT(); // Caso o ESP se desconecte do Broker, ele tenta se reconectar ao Broker
  }
  readData();
  MQTT.loop();
   
 
  delay(2000);
  
} //end loop


// ========================================================================================================
// --- Desenvolvimento das Funções ---
// ========================================================================================================

// --- Função para mostrar exibe leitura do sensor de temperatura ---
void imprimi_temperatura(){
  // Le a informacao do sensor
  sensors.requestTemperatures();
  tempC = sensors.getTempC(sensor1);
  
  // Atualiza temperaturas minima e maxima
  if (tempC < tempMin)
  {
    tempMin = tempC;
  }
  if (tempC > tempMax)
  {
    tempMax = tempC;
  }
  
  // Mostra dados no serial monitor
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Min : ");
  Serial.print(tempMin);
  Serial.print(" Max : ");
  Serial.println(tempMax);
  
  }


// --- Função para mostrar endereço do sensor ---
void mostra_endereco_sensor(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // Adiciona zeros se necessário
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// --- Função  ---
void printWifiData()
{
  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  Serial.print("MAC address: ");
  Serial.println(buf);
}

// --- Função  ---
void printCurrentNet()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to
  byte bssid[6];
  WiFi.BSSID(bssid);
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[5], bssid[4], bssid[3], bssid[2], bssid[1], bssid[0]);
  Serial.print("BSSID: ");
  Serial.println(buf);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.println(rssi);
}

// Função para se reconectar ao Broker MQTT caso cair a comunicação
void reconnectMQTT() {
  int i=0;
  while (!MQTT.connected() && i<10) {
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("MQTT conectado");    
    } else {
      Serial.println("MQTT reconectando");    
      i = i + 1;
      delay(1000);
    }
  }
}

void readData() {
    String payload = String(tempC);
    payload += ",";
    TTT = TTT + 1;
     payload += String(TTT);
    char payloadChar[payload.length()];
    payload.toCharArray(payloadChar, payload.length() + 1); // Converte a String para Char Array
    // Publica no tópico EMCOL todas as informações contidas na variável payload.
    MQTT.publish("fila", payloadChar);
  
}

// Funcão para se conectar ao Broker MQTT
void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Informa o broker e porta utilizados
}

 // Função para se reconectar a rede WiFi caso caia a comunicação com a rede
void recconectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
}
  
