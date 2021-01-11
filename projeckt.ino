#define COUNT_BUTTON_PIN  7 // <количество кнопок> - 1
#define BUTTON_SET_PIN    12
#define ENCODER_PINA      2
#define ENCODER_PINB      3
#define STEP              25
#define IN_ROUND          1000

#include <avr/eeprom.h>
#include "Arduino.h"
#include "NewEncoder.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4

const int8_t BUTTON_PIN[] = {4,5,6,7,8,9,10,11};
const uint16_t p[]={0,2,4,6,8,10,12,14,16,18};
uint8_t last_button = 0;
Adafruit_SSD1306 display(OLED_RESET);
String printValue; 
NewEncoder encoder(ENCODER_PINA, ENCODER_PINB, 0, 40, 0, FULL_PULSE);
int8_t prevEncoderValue;
int16_t n[COUNT_BUTTON_PIN+1];
bool set_button=false;

int16_t suming(int16_t *a) {
  int16_t b =0;
  for (int8_t i = 0; i<=COUNT_BUTTON_PIN; i++){
    b=a[i]+b;
  }
  return(b);
}

// функция вывода на экран текста b с размером s
void printOLED(int s, String b) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(s);
  display.println(b);
  display.display();
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(1,1);
  display.println("Go!");
  display.display();
  delay(1000);
  int16_t value, real_check_n;
  int16_t check_n;
  Serial.begin(9600);
  Serial.println("Start");
  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  encoder.begin(); // инициализация енкодера
  for (int i = 0; i<=COUNT_BUTTON_PIN; i++) { //инициализация пинов
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
  } 
  // чтение из памяти показателей n
  eeprom_read_block((void*)&n, 2, sizeof(n));
  check_n=eeprom_read_word(0);
  if (check_n != suming(n)) {
    for (int8_t i = 0; i <= COUNT_BUTTON_PIN; i++)
      n[i]=IN_ROUND;
    check_n = suming(n);
    Serial.println("запись в память");
    eeprom_write_word(0, check_n);
    eeprom_write_block((void*)&n, 2, sizeof(n));
  }
}

void loop() {
  float printcof;
  float cof = 1;
  bool up, down;
  int16_t checkN;
  uint8_t selected;
  uint8_t count_button=1;
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
    if (set_button){
     set_button=!set_button;
     display.invertDisplay(false);
    }
    for (int i = 0; i<=COUNT_BUTTON_PIN; i++) {
      if (!ClickButton[i]) cof=cof*float(n[i])/IN_ROUND; // подсчет коэффициента 
    }
    dtostrf(cof, 1, 3, print_cof); // преобразование коэффициента из float в строку вида x.xxx
    printOLED(4,print_cof); 
  } else 
  {
    display.invertDisplay(true);
    for (int8_t i = 0; i <= COUNT_BUTTON_PIN; i++) {
      if(!ClickButton[i]) count_button=count_button+1;
      }
    if (count_button!=1) printOLED(3,"ERROR");
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
      printOLED(4,print_cof);
      eeprom_read_block((void*)&read_n, 2, sizeof(read_n));
      if (read_n[selected]!=n[selected]){
        Serial.println("запись в память");
        eeprom_write_block((void*)&n, 2, sizeof(n));
        eeprom_write_word(0, suming(n));
      }
    }
  }
}
