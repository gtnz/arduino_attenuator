#define COUNT_BUTTON_PIN  7 // <количество кнопок> - 1
#define BUTTON_SET_PIN    12
#define ENCODER_PINA      2
#define ENCODER_PINB      3
#define STEP              25
#define IN_ROUND          1000
#define OLED_RESET 4
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#include <avr/eeprom.h>
#include "Arduino.h"
#include "NewEncoder.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/wdt.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
NewEncoder encoder(ENCODER_PINA, ENCODER_PINB, 0, 40, 0, FULL_PULSE);

const int8_t BUTTON_PIN[] = {4,5,6,7,8,9,10,11};  // пини с кнопками
// const uint16_t p[]={0,2,4,6,8,10,12,14,16,18};

String printUp;
String printDown;
uint8_t last_button = 0;
int8_t prevEncoderValue;
uint8_t parameter=0; // управление параметром
uint8_t R; // радиус
int16_t n[COUNT_BUTTON_PIN+1];
bool set_button=false;
bool mode = 0; // Режим ввода энергия/диаметр


int16_t suming(int16_t *a) {
  int16_t b =0;
  for (int8_t i = 0; i<=COUNT_BUTTON_PIN; i++){
    b=a[i]+b;
  }
  return(b);
}

// функция вывода на экран текста b с размером s
void printOLED(int s) {
  display.clearDisplay();
  display.setCursor(4,2);
  display.setTextColor(WHITE);
  display.setTextSize(s);
  display.println(printUp);
  display.setCursor(4,35);
  display.setTextSize(s);
  display.println(printDown);
  display.display();
}

void setup() {
  int16_t value, real_check_n;
  int16_t check_n;
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  delay(200);
  display.clearDisplay();
  display.setCursor(4,2);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.println("Starting");
  display.display();  
  Serial.println("Start");
  delay(400);
  encoder.begin(); // инициализация енкодера
  for (int i = 0; i<=COUNT_BUTTON_PIN; i++) { //инициализация пинов
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
  } 
  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  
  // чтение из памяти показателей n
  eeprom_read_block((void*)&n, 2, sizeof(n));
  check_n=eeprom_read_word(0);
  if (check_n != suming(n)) {
    for (int8_t i = 0; i <= COUNT_BUTTON_PIN; i++) {
      n[i]=IN_ROUND;
      Serial.println("запись в память");
    }
    check_n = suming(n);
    eeprom_write_word(0, check_n);
    eeprom_write_block((void*)&n, 2, sizeof(n));
  }
}

void loop() {
  wdt_enable(WDTO_1S); // через 1 секунду уйдет в рестарт, если не сбросится в конце лупа
  float printcof;
  float cof = 1;
  bool up, down;
  int16_t checkN;
  uint8_t selected;
  uint8_t count_button=0;
  bool ClickButton[COUNT_BUTTON_PIN+1];
  char print_cof[5];
  int16_t read_n[COUNT_BUTTON_PIN+1];
  up = encoder.upClick();
  down = encoder.downClick();

// опрос кнопок
  for (int8_t i = 0; i<=COUNT_BUTTON_PIN; i++) {
    ClickButton[i]=digitalRead(BUTTON_PIN[i]);
  }
  if (digitalRead(BUTTON_SET_PIN)) { // проверка кноки управления
    if (set_button){ // была ли нажата кнопка управления в прошлом такте
     set_button=!set_button;
     display.invertDisplay(false);
     
    }
    for (int i = 0; i<=COUNT_BUTTON_PIN; i++) {
      if (!ClickButton[i]) cof=cof*float(n[i])/IN_ROUND; // подсчет коэффициента 
    }
//    Serial.println(cof*10);
    dtostrf(cof, 1, 3, print_cof); // преобразование коэффициента из float в строку вида x.xxx
    printUp = print_cof;
    printOLED(4); 
  } else 
  {
    if (!set_button){ // была ли нажата кнопка управления в прошлом такте
      set_button=!set_button;
      display.invertDisplay(true);
    }
    for (int8_t i = 0; i <= COUNT_BUTTON_PIN; i++) {
      if(!ClickButton[i]) count_button=count_button+1;
      }
    if (count_button!=1) {
      display.clearDisplay();
      display.setCursor(4,2);
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.println("ERROR 01");      
      display.display(); 
      set_button=true;
    }
    else {
      for (int8_t i = 0; i <= COUNT_BUTTON_PIN; i++) {
        if (!ClickButton[i]) {
          selected=i;
          break;
        }
      }
      if (!set_button || last_button != selected ) {   
        encoder=n[selected]/STEP;
        set_button=true;
      }
      last_button=selected;
      n[selected]=encoder*STEP;
      cof = float(n[selected])/IN_ROUND;
      dtostrf(cof, 1, 3, print_cof);
      printDown= print_cof;
      printOLED(4);
      eeprom_read_block((void*)&read_n, 2, sizeof(read_n));
      if (read_n[selected]!=n[selected]){
        Serial.println("запись в память");
        eeprom_write_block((void*)&n, 2, sizeof(n));
        eeprom_write_word(0, suming(n));
      }
    }
  }
  wdt_reset(); // сброс таймера на рестарт
}
