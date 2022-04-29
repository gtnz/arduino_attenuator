// Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
#include <Arduino.h>
#include <U8g2lib.h>
#include <avr/eeprom.h>
#include <SPI.h>
// #include "NewEncoder.h"

#define ENCODER_PINA      2
#define ENCODER_PINB      3
#define STEP              25
#define IN_ROUND          1000
#define BUTTON_SET_PIN    12
#define DEOD_PIN          A6
#define MIRROR            1

// NewEncoder encoder(ENCODER_PINA, ENCODER_PINB, 0, 10000, 0, FULL_PULSE); 
U8G2_ST7565_ERC12864_1_4W_SW_SPI u8g2(U8G2_R0, /* scl=*/ 5, /* si=*/ 11, /* cs=*/ 10, /* rs=*/ 9, /* rse=*/ 8);

uint16_t e;
uint8_t control = 0;
bool set_button=false;
float cof;

// Для энкодера
#define btn_long_push 1000   // Длительность долинного нажатия кнопки
volatile uint8_t lastcomb=7, enc_state, btn_push=0;
volatile int enc_rotation=0, btn_enc_rotate=0;
volatile boolean btn_press=0;
volatile uint32_t timer;

void setup(void) {
//  encoder.begin();
  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  u8g2.begin(); 
  u8g2.setContrast (0); 
  u8g2.enableUTF8Print();
  e = eeprom_read_float(0);
// Для энкодера
  pinMode(A1,INPUT_PULLUP); // ENC-A
  pinMode(A2,INPUT_PULLUP); // ENC-B
  pinMode(A3,INPUT_PULLUP); // BUTTON(энкодер)

  pinMode(DEOD_PIN, INPUT); //аналоговый вход для диода

  PCICR =  0b00000010; // PCICR |= (1<<PCIE1); Включить прерывание PCINT1
  PCMSK1 = 0b00001110; // Разрешить прерывание для  A1, A2, A3
}
 
void loop(void) {
  uint16_t read_e;
  char print_cof[5]="";
  delay(100);
  if (digitalRead(BUTTON_SET_PIN)) { // если кнопка управления нажата
    if (!set_button){ // была ли нажата кнопка управления в прошлом такте
      set_button=!set_button;
      enc_rotation=e;
    }
    e=enc_rotation;
    }
  else {
    if (set_button){ // была ли нажата кнопка управления в прошлом такте
    set_button=!set_button;  
    read_e=eeprom_read_float(0);
    if (read_e!=e){
        Serial.println("запись в память");
        eeprom_write_float(0,e);
      }
    }
  }
  cof = 1.0*e*STEP/IN_ROUND;
  Serial.println(set_button);
  dtostrf(cof, 1, 3, print_cof);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(0, 20);
    u8g2.print(print_cof);
  } while ( u8g2.nextPage() );
}

// Для энкодера
//****************************************
ISR (PCINT1_vect) //Обработчик прерывания от пинов A1, A2, A3
{
  uint8_t comb = bitRead(PINC, 3) << 2 | bitRead( PINC, 2)<<1 | bitRead(PINC, 1); //считываем состояние пинов энкодера и кнопки

 if (comb == 3 && lastcomb == 7) btn_press=1; //Если было нажатие кнопки, то меняем статус
 
 if (comb == 4)                         //Если было промежуточное положение энкодера, то проверяем его предыдущее состояние 
 {
    if (lastcomb == 5) --enc_rotation; //вращение по часовой стрелке
    if (lastcomb == 6) ++enc_rotation; //вращение против частовой
    enc_state=1;                       // был поворот энкодера    
    btn_enc_rotate=0;                  //обнулить показания вращения с нажатием
  }
  
   if (comb == 0)                      //Если было промежуточное положение энкодера и нажатие, то проверяем его предыдущее состояние 
   {
    if (lastcomb == 1) --btn_enc_rotate; //вращение по часовой стрелке
    if (lastcomb == 2) ++btn_enc_rotate; //вращение против частовой
    enc_state=2;                        // был поворот энкодера с нажатием  
    enc_rotation=0;                     //обнулить показания вращения без нажатия
    btn_press=0;                         //обнулить показания кнопки
   }

   if (comb == 7 && lastcomb == 3 && btn_press) //Если было отпусание кнопки, то проверяем ее предыдущее состояние 
   {
    if (millis() - timer > btn_long_push)         // проверяем сколько прошло миллисекунд
    {
      enc_state=4;                              // было длинное нажатие 
    } else {
             enc_state=3;                    // было нажатие 
            }
      btn_press=0;                           //обнулить статус кнопки
    }
   
  timer = millis();                       //сброс таймера
  lastcomb = comb;                        //сохраняем текущее состояние энкодера
}
