#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ServoSmooth.h>
//Логин- пароль wifi
#define ssid  "Keenetic-8995"
#define password  "S1e9r8g5ey"

#define AMOUNT 4 //количество сервоприводов

ServoSmooth servos[AMOUNT]; // инициализируем библиотеку
//переменные для таймеров
uint32_t servoTimer;
int position1 = 0;   // стартовая позиция
int position2 = 90;  // конечная позиция
boolean flag;

ESP8266WebServer server(80);


String webPage = "";

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

void move_servo (int num, int deg)
{
  servos[num].setTargetDeg(deg);  
}
void loop() {
  server.handleClient();
  // каждые 20 мс
  if (millis() - servoTimer >= 20) {  // опрос серво каждые 20 мс
    servoTimer += 20;
    for (byte i = 0; i < AMOUNT; i++) {
      servos[i].tickManual();   // двигаем все сервы при необходимости
    }
}
}