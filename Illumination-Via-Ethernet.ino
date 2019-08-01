/*********************************************************************
 * Управление освещением по Ethernet
 * Платформа: Arduino UNO
 * Сетевые настройки: IP  - 192.168.1.30
 *                    Mac - 02:AA:BB:CC:00:22
 * Подключенные модули: Светодиодная лента - пин: 6, 5, 3 
 *                      Двухканальное реле - пин: 4, 2
 *                      Ethernet Shield - пин: 13, 12, 11, 10
 * Управление: Люстра - Кнопки "Правый выключатель" и "Левый выключатель"          
 *                      посылают на контроллер значение с номером пина на 
 *                      котором находится реле (2 и 4). После сбрасывает 
 *                      значение в 0.
 *             Светодиодная лента - После нажатия кнопки "Старт" присваивает 
 *                                  change_colors значение 1 и в авторежиме 
 *                                  по очереди проходит по всем цветам
 *                                  (R, G, B, RG, RB, GB, RGB) и так по 
 *                                  кругу пока флаг change_colors не будет 
 *                                  равен нулю.
 *                                  Кнопка "Стоп" присваивает change_colors
 *                                  значение 0.
 *********************************************************************/

#include "SPI.h"
#include "Ethernet.h" //поменять в хидере вебдуино на ethernet.h когда пойдет на опытный образец (ethernet2.h для амперковского шилда с wiznet 5500)
#include "WebServer.h"

#define PREFIX "/light"
#define RED_PIN 6
#define GREEN_PIN 5
#define BLUE_PIN 3
#define RIGHT_RELAY_PIN 4
#define LEFT_RELAY_PIN 2

WebServer webserver(PREFIX, 80);

static uint8_t mac[6] = { 0x02, 0xAA, 0xBB, 0xCC, 0x00, 0x22 };
static uint8_t ip[4] = { 192, 168, 1, 30 }; // IP адрес Arduino.

int red = 0; //Переменные для изменения цвета через слайдеры 
int blue = 0;          
int green = 0; 
         
int relay_toggle = 0;
int change_colors = 0; //Флаг для ВКЛ/ВЫКЛ авторежима
int color_state_counter = 0; //Счетчик для авторежима

void lightCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      repeat = server.readPOSTparam(name, 16, value, 16);
      
      if (strcmp(name, "red") == 0)
        red = strtoul(value, NULL, 10);
        
      if (strcmp(name, "green") == 0)
        green = strtoul(value, NULL, 10);
        
      if (strcmp(name, "blue") == 0)
        blue = strtoul(value, NULL, 10);
        
      if (strcmp(name, "relay_toggle") == 0)
        relay_toggle = strtoul(value, NULL, 10);
        
      if (strcmp(name, "change_colors") == 0)
        change_colors = strtoul(value, NULL, 10);
        
    } while (repeat);
    
    server.httpSeeOther(PREFIX);
    return;
  }
  server.httpSuccess();

  if (type == WebServer::GET)
  {
    P(message) = 
"<!DOCTYPE html><html><head>"
  "<meta charset=\"utf-8\"><meta name=\"apple-mobile-web-app-capable\" content=\"yes\" /><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge,chrome=1\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\">"
  "<title>Управление освещением</title>"
  "<link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.0/jquery.mobile-1.0.min.css\" />"
  "<script src=\"http://code.jquery.com/jquery-1.6.4.min.js\"></script>"
  "<script src=\"http://code.jquery.com/mobile/1.0/jquery.mobile-1.0.min.js\"></script>"
  "<style> body, .ui-page { background: black; } .ui-body { padding-bottom: 1.5em; } div.ui-slider { width: 88%; } #red, #green, #blue { display: block; margin: 10px; } #red { background: #f00; } #green { background: #0f0; } #blue { background: #00f; } </style>"
  "<script>"
     "$(document).ready(function(){ $('#red, #green, #blue').slider; $('#red, #green, #blue').bind( 'change', function(event, ui) { jQuery.ajaxSetup({timeout: 110}); var id = $(this).attr('id'); var strength = $(this).val(); if (id == 'red') $.post('/light', { red: strength } ); if (id == 'green') $.post('/light', { green: strength } ); if (id == 'blue') $.post('/light', { blue: strength } );  }); });"
     "$(function() { $('#relay_toggle').button(); $('#relay_toggle_left, #relay_toggle_right').click( function(event) { jQuery.ajaxSetup({timeout: 110}); var id = $(this).attr('id');  if (id == 'relay_toggle_left') $.post('/light', { relay_toggle: 2 } ); if (id == 'relay_toggle_right') $.post('/light', { relay_toggle: 4 } ); }); });"
     "$(function() { $('#change_colors').button(); $('#change_colors_start, #change_colors_stop').click( function(event) { jQuery.ajaxSetup({timeout: 110}); var id = $(this).attr('id');  if (id == 'change_colors_start') $.post('/light', { change_colors: 1 } ); if (id == 'change_colors_stop') $.post('/light', { change_colors: 0 } ); }); });"
  "</script>"
"</head>"
"<body>"
  "<div data-role=\"header\" data-position=\"inline\"><h1>Люстра</h1></div>" 
  "<div class=\"ui-body ui-body-a\">"
    "<button name=\"button\" id=\"relay_toggle_left\">Левый выключатель</button>"
    "<button name=\"button\" id=\"relay_toggle_right\">Правый выключатель</button>"
  "</div>"
  "<div data-role=\"header\" data-position=\"inline\"><h1>Светодиодная лента(Manual)</h1></div>"
    "<div class=\"ui-body ui-body-a\">"
      "<input type=\"range\" name=\"slider\" id=\"red\" value=\"0\" min=\"0\" max=\"255\"  />"
      "<input type=\"range\" name=\"slider\" id=\"green\" value=\"0\" min=\"0\" max=\"255\"  />"
      "<input type=\"range\" name=\"slider\" id=\"blue\" value=\"0\" min=\"0\" max=\"255\"  />"
    "</div>"
    "<div data-role=\"header\" data-position=\"inline\"><h1>Светодиодная лента(Auto)</h1></div>"
    "<div class=\"ui-body ui-body-a\">"
      "<button name=\"button\" id=\"change_colors_start\">Старт</button>"
      "<button name=\"button\" id=\"change_colors_stop\">Стоп</button>"
    "</div>"
  "</body>"
"</html>";
    server.printP(message);
  }
}

void setup()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(RIGHT_RELAY_PIN, OUTPUT);
  pinMode(LEFT_RELAY_PIN, OUTPUT);

  digitalWrite(RIGHT_RELAY_PIN, 1);
  digitalWrite(LEFT_RELAY_PIN, 1);
  
  
  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&lightCmd);
  webserver.begin();
}

void loop()
{
  BEGIN: //Метка для возврата из авторежима ленты
  webserver.processConnection();
  
  if (relay_toggle > 0)
  {
    digitalWrite(relay_toggle, !digitalRead(relay_toggle)); 
    relay_toggle = 0;
  }
 
  if (change_colors == 1) //Переход в авто режим если фалаг change_colors > 0 (Нажатие кнопки "Старт")
  {
    /*********************  КРАСНЫЙ  ***********************************************/
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает красный от 0 до 5
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      delay(50);
    }

    for (int i = 0; i <= 100; i++)      //Ждем
    {
      webserver.processConnection();     //Проверяю не нажата ли кнопка "Стоп"
      if (change_colors == 0)            //Если нажата - вырубает ленту и переходит в начало цикла "loop"
      {                                  //и так на каждом цвете.
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает красный от 5 до 0
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      delay(50);
    }
    
    /*********************  ЗЕЛЕНЫЙ  ***********************************************/
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает зеленый от 0 до 5
    {
      analogWrite(GREEN_PIN, color_state_counter);
      delay(50);
    }
    
    for (int i = 0; i <= 100; i++) 
    {
      webserver.processConnection();
      if (change_colors == 0)
      {
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает зеленый от 5 до 0
    {
      analogWrite(GREEN_PIN, color_state_counter);
      delay(50);
    }
    /*********************  СИНИЙ  ***********************************************/  
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает синий от 0 до 5
    { 
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }

    for (int i = 0; i <= 100; i++) 
    {
      webserver.processConnection();
      if (change_colors == 0)
      {
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает синий от 5 до 0
    { 
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }
    /*********************  КРАСНЫЙ + ЗЕЛЕНЫЙ  ***********************************/
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает красный и зеленый от 0 до 5
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      analogWrite(GREEN_PIN, color_state_counter);
      delay(50);
    }

    for (int i = 0; i <= 100; i++) 
    {
      webserver.processConnection();
      if (change_colors == 0)
      {
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает красный и зеленый от 5 до 0
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      analogWrite(GREEN_PIN, color_state_counter);
      delay(50);
    }
    /*********************  КРАСНЫЙ + СИНИЙ  ***********************************/
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает красный и синий от 0 до 5
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }

    for (int i = 0; i <= 100; i++) 
    {
      webserver.processConnection();
      if (change_colors == 0)
      {
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает красный и синий от 5 до 0
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }
    /*********************  ЗЕЛЕНЫЙ + СИНИЙ  ***********************************/
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает зеленый и синий от 0 до 5
    {                                           
      analogWrite(GREEN_PIN, color_state_counter);
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }

    for (int i = 0; i <= 100; i++) 
    {
      webserver.processConnection();
      if (change_colors == 0)
      {
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает зеленый и синий от 5 до 0
    {                                           
      analogWrite(GREEN_PIN, color_state_counter);
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }
    /*********************  КРАСНЫЙ + ЗЕЛЕНЫЙ + СИНИЙ  **************************/
    for (color_state_counter = 0; color_state_counter <= 5; color_state_counter++) //Накручивает красный, зеленый и синий от 0 до 5
    {                                           
      analogWrite(RED_PIN, color_state_counter);
      analogWrite(GREEN_PIN, color_state_counter);
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }

    for (int i = 0; i <= 100; i++) 
    {
      webserver.processConnection();
      if (change_colors == 0)
      {
        goto BEGIN;
      }
      delay(50);
    }
    
    for (color_state_counter = 5; color_state_counter >= 0; color_state_counter--) //Cкручивает красный, зеленый и синий от 5 до 0
    { 
      analogWrite(RED_PIN, color_state_counter);                                          
      analogWrite(GREEN_PIN, color_state_counter);
      analogWrite(BLUE_PIN, color_state_counter);
      delay(50);
    }    
  }
  else //Переход в режим слайдеров если фалаг change_colors = 0 (Нажатие кнопки "Стоп")
  {
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);  
  }  
}
