#include <ESP8266WiFi.h> //работа с wifi
#include <ESP8266WebServer.h> //веб сервер
#include <PubSubClient.h> //клиент mqtt
#include <Servo.h> //работа с серво
//Логин- пароль wifi
#define ssid  "ssid" //точка доступа wifi
#define password  "pass" //пароль wifi
#define MSG_BUFFER_SIZE (50) //размер буфера для сообщений mqtt

Servo servo; // инициализируем библиотеку
int8_t speed=1; //скорость движения

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
 uint8_t param; //параметры серво
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i = 0; i < length; i++) {
 message = message+(char)payload[i];
 }
 Serial.println(message);//общий вид сообщения "НомерСерво Угол"
 param=message.toInt();//угол поворота
Serial.println(param);
 move_servo (param);
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
void move_servo (int deg)
{
   int cur_pos=servo.read();
     if (deg<=180 && deg>=0) //проверка корректности полученных значений
  {
    if (cur_pos<deg)
      {
    for (int pos = cur_pos; pos <= deg; pos += speed) 
        {     servo.write(pos);              
              delay(15);                      
        }
      }
    else
      {
      for (int pos = cur_pos; pos >= deg; pos -= speed) 
            servo.write(pos);
            delay(15);
      }                       
    }
  else
  {
    Serial.println("Incorrect value");
    Serial.println(deg);
  }
}

//функция обработчик команды веб серверу
void handle_servomove()
{
int deg=server.arg("angle").toInt();
  Serial.println(deg);
move_servo(deg);
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
  WebPage += "Угол<br>";
  WebPage += "<input type='range' id='volume' name='angle' min='0' max='180' /><br>";
  WebPage += "<input type='button' value='Применить' onclick=\"location.href='/servomove?angle='+angle.value\">";
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
  
  servo.attach(D3,540,2400);
  
Serial.println("Servos Attached");
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
}