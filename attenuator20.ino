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
#define DEOD_PIN          7
#define MIRROR            1
#define IN_SEC            4000

// NewEncoder encoder(ENCODER_PINA, ENCODER_PINB, 0, 10000, 0, FULL_PULSE); 
U8G2_ST7565_ERC12864_1_4W_SW_SPI u8g2(U8G2_R0, /* scl=*/ 11, /* si=*/ 12, /* cs=*/ 8, /* rs=*/10 , /* rse=*/9 );

uint16_t e, cof_save;
uint8_t control = 0;
bool set_button=false;
float cof, e_final, zero;

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
  cof_save = eeprom_read_float(0);  // чтение сохраненного коэфициента
// Для энкодера
  pinMode(A1,INPUT_PULLUP); // ENC-A
  pinMode(A2,INPUT_PULLUP); // ENC-B
  pinMode(A3,INPUT_PULLUP); // BUTTON(энкодер)

  pinMode(DEOD_PIN, INPUT); //аналоговый вход для диода

  PCICR =  0b00000010; // PCICR |= (1<<PCIE1); Включить прерывание PCINT1
  PCMSK1 = 0b00001110; // Разрешить прерывание для  A1, A2, A3
  Serial.println("start!");
  // Подсчет нуля
  for (int i=0; i <= 8000; i++) {
      zero = zero+analogRead(DEOD_PIN);
  }
    zero = zero/8000-2;
    Serial.println(zero);
}
 
void loop(void) {
  float cof, e_final_1, e_summ;
  int sec[4000];
  uint16_t read_e, endpulse=0;
  uint32_t pulse = 0;
  char print_number[5]="";
  String print_line_2="";
  String print_line_1="";
//  delay(100);
  if (digitalRead(BUTTON_SET_PIN)) { // если кнопка управления нажата
    if (!set_button){ // была ли нажата кнопка управления в прошлом такте
      set_button=!set_button;
      enc_rotation=cof_save;
    }
    cof_save=enc_rotation;
    print_line_2="коэффициент";
    print_line_1="";
    cof = 1.0*cof_save*STEP/IN_ROUND;
    dtostrf(cof, 1, 3, print_number);
    }
  else {                            // кнопка управления не нажата
    if (set_button){                // была ли нажата кнопка управления в прошлом такте
    set_button=!set_button;  
    read_e=eeprom_read_float(0);
    if (read_e!=cof_save){
        Serial.println("запись в память");
        eeprom_write_float(0,cof_save);
      }
    }
    print_line_2=String("МДж/cм");
    print_line_1="2";
    cof = 1.0*cof_save*STEP/IN_ROUND;
    e_summ=0;
    Serial.println(e_summ);
    for (int i=0; i <= IN_SEC; i++) {
      sec[i] = analogRead(DEOD_PIN);
    }
    pulse = 0;
    for (int i=0; i <=IN_SEC; i++) {
      if (sec[i]>zero*1.25){            // обнаружение импульса 
        endpulse=i+100; 
        while (sec[i]>zero*1.25) {
          pulse = pulse + sec[i];
          i=i+1;
        }
      e_summ=e_summ+pulse/100;
      }
    }
    Serial.println(e_summ);
    Serial.println(e);
    e = 1.0*e/MIRROR;
    e_final_1 = 1.0*e_summ/16000-zero; // для настройки 
    if (abs(e_final_1-e_final)>0) {
      e_final = e_final_1; // для настройки 
//      e_final = e_summ*cof;
    }
    if (e_final<3) {
      e_final = 0;
    }
    dtostrf(e_final, 1, 1, print_number);
  }
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_logisoso32_tn);
    u8g2.setCursor(0, 35);
    u8g2.print(print_number);
    u8g2.setFont(u8g2_font_unifont_t_cyrillic);
    u8g2.setCursor(0, 48);
    u8g2.print(print_line_2);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(48, 43);
    u8g2.print(print_line_1);    
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
