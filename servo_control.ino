#include <ESP8266WiFi.h> //работа с wifi
#include <ESP8266WebServer.h> //веб сервер
#include <PubSubClient.h> //клиент mqtt
#include <ServoSmooth.h> //работа с серво
//Логин- пароль wifi
#define ssid  "ssid" //точка доступа wifi
#define password  "pass" //пароль wifi
#define AMOUNT 2 //количество сервоприводов
#define MAXANGLE 90 //максимальный угол поворота
#define MSG_BUFFER_SIZE (50) //размер буфера для сообщений mqtt

ServoSmooth servos[AMOUNT]; // инициализируем библиотеку

//переменные для таймеров
uint32_t servoTimer;

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
String WebPage = "";

//Функция чтения и обработки сообщений из топика
void callback(char* topic, byte* payload, unsigned int length) 
{
 String message; //для хранения сообщения топика
 uint8_t param1, param2; //параметры серво
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i = 0; i < length; i++) {
 message = message+(char)payload[i];
 }
 Serial.println(message);//общий вид сообщения "НомерСерво Угол"

 param1=message.substring(0,1).toInt();//Номер привода
 param2=message.substring(2).toInt();//угол поворота
 move_servo (param1, param2);
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
 client.publish("StatusTopic", "Device Started");
 // и подписываемся на топик управляющих команд
 client.subscribe("CmdTopic");
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
  if (num<=AMOUNT && deg<=MAXANGLE && deg>=0) //проверка корректности полученных значений
  {
  if (num<AMOUNT)
      {
        servos[num].setTargetDeg(deg); //двигаем указанную серву
      }
  else 
      {
        for (byte i = 0; i < AMOUNT; i++) {
            servos[i].setTargetDeg(deg);   // двигаем все сервы
          }  
      }
  }
  else
  {
    Serial.print("Incorrect values");
    Serial.println(num);
    Serial.println(deg);
  }  
}

//функция обработчик команды веб серверу
void handle_servomove()
{
int num=server.arg("servonum").toInt();
int deg=server.arg("angle").toInt();
  Serial.println(num);
  Serial.println(deg);
move_servo(num,deg);
server.send(200, "text/plain", "OK");
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
  //Генерация веб интерфейса
  WebPage += "<!DOCTYPE html>";
  WebPage += "<html lang='ru'>";
  WebPage += "<head>";
  WebPage += "<meta http-equiv='Content-type' content='text/html; charset=utf-8'>";
  WebPage += "<title>Контроль серво</title>";
  WebPage += "</head><body>";
  WebPage += "<h1>Управление воздушными заслонками</h1>";
  WebPage += "<form method='get' action='/servomove'>";
  WebPage += "<select name=\"servo\">";
  for (byte i = 0; i < AMOUNT; i++) 
       {
        WebPage +="<option value=\""+String(i)+"\">Клапан "+String(i)+"</option>";
       }
  WebPage += "<option value=\""+ String(AMOUNT)+"\">Все</option>";
  WebPage += "</select><br>";
  WebPage += "Угол<br>";
  WebPage += "<input type='range' id='volume' name='angle' min='0' max='"+String (MAXANGLE)+"' /><br>";
  WebPage += "<input type='button' value='Применить' onclick=\"location.href='/servomove?servonum='+servo.value+';angle='+angle.value\">";
  WebPage += "</form>";
  WebPage += "</body></html>";
   server.on("/", []() {
    server.send(200, "text/html", WebPage);
   });
 
   server.on("/servomove", handle_servomove);

  server.begin();
  Serial.println("Server started");
  Serial.println("Attaching servo...");
  //подключаем серво
  
  servos[0].attach(D3);
  servos[0].smoothStart();
  servos[1].attach(D4);
  servos[1].smoothStart();
  //servos[2].attach(5);
  //servos[2].smoothStart();
  //servos[3].attach(6);
  //servos[3].smoothStart();
  
Serial.println("Servos Attached");
  //Настройки скорости и ускорений для каждого из серво
Serial.println("Setting up servos...");
  for (byte i = 0; i < AMOUNT; i++) {
      servos[i].setSpeed(90);   // скорость градусов в секунду
      servos[1].setAccel(0.1); //ускорение (от 0 до 1)
    }
Serial.println("All done!");    
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
      for (byte i = 0; i < AMOUNT; i++) {
      bool move=servos[i].tick();   // двигаем все сервы при необходимости
      Serial.println(move);
    }
}
