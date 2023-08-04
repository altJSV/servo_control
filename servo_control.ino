#include <ESP8266WiFi.h> //работа с wifi
#include <ESP8266WebServer.h> //веб сервер
#include <PubSubClient.h> //клиент mqtt
#include <ServoSmooth.h> //работа с серво
//Логин- пароль wifi
#define ssid  "ssid" //точка доступа wifi
#define password  "pass" //пароль wifi
#define AMOUNT 4 //количество сервоприводов
#define MSG_BUFFER_SIZE (50) //размер буфера для сообщений mqtt

ServoSmooth servos[AMOUNT]; // инициализируем библиотеку

//переменные для таймеров
uint32_t servoTimer;
int position1 = 0;   // стартовая позиция
int position2 = 90;  // конечная позиция

//mqtt
const char* mqtt_server = "192.168.1.1"; //ip или http адрес
int mqtt_port = 1883; //порт
const char* mqtt_login="login"; //логин
const char* mqtt_pass="pass"; //пароль

ESP8266WebServer server(80); //резервируем для вебсервера 80 порт
WiFiClient espClient; //инициализируем wifi клиент
PubSubClient client(espClient);//инициализируем mqtt клиент

//объявляем переменные для получения сообщений mqtt
unsigned long lastMsg = 0; 
char msg[MSG_BUFFER_SIZE];

//Переменная для генерации веб страницы
String webPage = "";

//Функция чтения и обработки сообщений из топика
void callback(char* topic, byte* payload, unsigned int length) 
{
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i = 0; i < length; i++) {
 Serial.print((char)payload[i]);
 }
 Serial.println();

 // Switch on the LED if an 1 was received as first character
 if ((char)payload[0] == '1') {
 digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
 // but actually the LED is on; this is because
 // it is active low on the ESP-01)
 } else {
 digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
 }
}

//функция переподключения к mqtt
 void reconnect() 
 {
 // пытаемся подключиться к серверу пока не надоест
 while (!client.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Генерируем ID клиента
 String clientId = "ESP8266Client-";
 clientId += String(random(0xffff), HEX);
 // Attempt to connect
 if (client.connect(clientId.c_str(),mqtt_login,mqtt_pass)) {
 Serial.println("connected");
 // Как только подключились пишем в топик, что все хорошо
 client.publish("outTopic", "hello world");
 // и подписываемся на топик управляющих команд
 client.subscribe("inTopic");
 } else {
 Serial.print("failed, rc=");
 Serial.print(client.state());
 Serial.println(" try again in 5 seconds");
 // ждем 5 секунд перед повторным подключением
 delay(5000);
 }
 }
}

//Функция движения серво
void move_servo (int num, int deg)
{
  servos[num].setTargetDeg(deg);  
}

void setup() {
  Serial.begin(115200);
  delay(100);
 
  // Подключаемся к wifi
 
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected"); 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //Подключаемся к mqtt
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  webPage += "<h1>Dog Rx Machine</h1><p>AM Medicine  <a href=\"deliver\"><button>Deliver</button></a>&nbsp;<a href=\"reset\"><button>Reset</button></a></p>";
  webPage += "<h2> </h1><p>PM Medicine  <a href=\"deliver\"><button>Deliver</button></a>&nbsp;<a href=\"reset\"><button>Reset</button></a></p>";
  webPage += "<h3> </h1><p>Treat  <a href=\"deliver\"><button>Deliver</button></a>&nbsp;<a href=\"reset\"><button>Reset</button></a></p>";
  webPage += "<h4> </h1><p>Kong Toy  <a href=\"deliver\"><button>Deliver</button></a>&nbsp;<a href=\"reset\"><button>Reset</button></a></p>";
   server.on("/", []() {
    server.send(200, "text/html", webPage);
   });
 
   server.on("/deliver", []() {
    server.send(200, "text/html", webPage);
    Serial.println("HTTP OPEN COMMAND RECEIVED");
  });
 
  server.on("/reset", []() {
    server.send(200, "text/html", webPage);
    Serial.println("HTTP CLOSE COMMAND RECEIVED");
  });

  server.begin();
  Serial.println("Server started");

  //подключаем серво
  servos[0].attach(3);
  servos[1].attach(4);
  servos[2].attach(5);
  servos[3].attach(6);

  //Настройки скорости и ускорений для каждого из серво
  for (byte i = 0; i < AMOUNT; i++) {
      servos[i].setSpeed(90);   // скорость градусов в секунду
      servos[1].setAccel(0.2); //ускорение (от 0 до 1)
    }
}

void loop() {
  server.handleClient();//пинаем веб сервер
  //проверяем подключение к MQTT
  if (!client.connected()) 
  {
     reconnect();//Пробуем переподключится
  }
      client.loop();//пинаем MQTT клиент
  
  // каждые 20 мс опрашиваем серво
  if (millis() - servoTimer >= 20) {  // опрос серво каждые 20 мс
    servoTimer += 20;
    for (byte i = 0; i < AMOUNT; i++) {
      servos[i].tickManual();   // двигаем все сервы при необходимости
    }
}
}