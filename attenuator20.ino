// Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
#include <Arduino.h>
#include <U8g2lib.h>
#include <avr/eeprom.h>
#include <SPI.h>

#define ENCODER_PINA      2
#define ENCODER_PINB      3
#define STEP              25
#define IN_ROUND          1000
#define BUTTON_SET_PIN    12
#define DEOD_PIN          7
#define MIRROR            1
#define IN_SEC            1000

U8G2_ST7565_ERC12864_1_4W_SW_SPI u8g2(U8G2_R0, /* scl=*/ 11, /* si=*/ 12, /* cs=*/ 8, /* rs=*/10 , /* rse=*/9 );

bool set_button=false;
float e=0, zero;

void setup(void) {
//  encoder.begin();
  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  u8g2.begin(); 
  u8g2.setContrast (0); 
  u8g2.enableUTF8Print();
// Для энкодера
  pinMode(A1,INPUT_PULLUP); // ENC-A
  pinMode(A2,INPUT_PULLUP); // ENC-B
  pinMode(A3,INPUT_PULLUP); // BUTTON(энкодер)

  pinMode(DEOD_PIN, INPUT); //аналоговый вход для диода

  PCICR =  0b00000010; // PCICR |= (1<<PCIE1); Включить прерывание PCINT1
  PCMSK1 = 0b00001110; // Разрешить прерывание для  A1, A2, A3
  Serial.println("start!");
  // Подсчет нуля
  for (int i=0; i <= 4000; i++) {
      zero = zero+analogRead(DEOD_PIN)/4;
//      Serial.println(i);
  }
    zero = zero/4000;
    Serial.println(zero);
    Serial.println("____________");
}
 
void loop(void) {
  float e_summ;
  uint8_t sec[IN_SEC];
  uint8_t count_pulse=0;
  uint16_t read_e, endpulse=0;
  uint32_t pulse = 0;
  uint16_t pulse_L = 0;
  char print_number[5]="";
  bool start=false;
  zero = 10;  
  e_summ=0;
  Serial.println("-----");
  delay(1);
  for (int j=0; j < 5; j++) {
    for (int i=0; i < IN_SEC; i++) {
      sec[i] = analogRead(DEOD_PIN)/4;
    }
    Serial.println("=====");
    pulse = 0;
    Serial.println(sec[1]);
    int i=0;
    if (sec[i]>zero /*&& false*/) {               
      while (sec[i]>zero*1.25 && i<IN_SEC) { // отбрасывание куска импульса в начале;
        i++;
      }
      Serial.print("drop ");
      Serial.print(i);
    }
    while (i <=IN_SEC-50) {
      if (sec[i]>zero*1.25 && i<=IN_SEC){            // обнаружение импульса
        count_pulse=count_pulse+1;
        while (sec[i]>zero*1.25 && i<IN_SEC) {
          pulse_L++;
          pulse = pulse + sec[i];
          i++;
        }
        pulse=pulse/pulse_L;
        e_summ=e_summ+pulse/count_pulse;
        Serial.println("Pulse_L ");
        Serial.println(pulse_L);
        pulse_L=0;
      }
      i++;
    }
  }
  Serial.println(e_summ);
  if (abs(1.0*e_summ/MIRROR-e)>5){
  e = 1.0*e_summ/MIRROR;
  }
  if (e<3) {
    e = 0;
  }
  Serial.println(count_pulse);
  dtostrf(e, 1, 1, print_number);
  Serial.println(print_number);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_logisoso32_tn);
    u8g2.setCursor(0, 35);
    u8g2.print(print_number);
    u8g2.setFont(u8g2_font_unifont_t_cyrillic);
    u8g2.setCursor(0, 48);
    u8g2.print(String("МДж/cм"));
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(48, 43);
    u8g2.print("2");    
  } while ( u8g2.nextPage() );
}
