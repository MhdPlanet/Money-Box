


//-------Setting---------
#define coin_amount 4    
float coin_value[coin_amount] = {1.0, 2.0, 5.0, 10.0};  
String currency = "RUB";
int stb_time = 20000;    

int coin_signal[coin_amount];    
int coin_quantity[coin_amount]; 
byte empty_signal;               
unsigned long standby_timer, reset_timer;
float summ_money = 0;           


#include "LowPower.h"
#include "EEPROMex.h"
#include "LCD_1602_RUS.h"


LCD_1602_RUS lcd(0x3f, 16, 2);         
// 0x27 на 0x3f

boolean recogn_flag, sleep_flag = true;   
//-----
#define button 2         
#define calibr_button 3  
#define disp_power 12    
#define LEDpin 11        
#define IRpin 17         
#define IRsens 14        
//-------
int sens_signal, last_sens_signal;
boolean coin_flag = false;

void setup() {
  Serial.begin(9600);                   
  delay(500);

  // 
  pinMode(button, INPUT_PULLUP);
  pinMode(calibr_button, INPUT_PULLUP);

  // 
  pinMode(disp_power, OUTPUT);
  pinMode(LEDpin, OUTPUT);
  pinMode(IRpin, OUTPUT);

  // 
  digitalWrite(disp_power, 1);
  digitalWrite(LEDpin, 1);
  digitalWrite(IRpin, 1);

  // 
  attachInterrupt(0, wake_up, CHANGE);

  empty_signal = analogRead(IRsens); 
  
  lcd.init();
  lcd.backlight();

  if (!digitalRead(calibr_button)) {  
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(L"Service");
    delay(500);
    reset_timer = millis();
    while (1) {                                 
      if (millis() - reset_timer > 3000) {       
     
        for (byte i = 0; i < coin_amount; i++) {
          coin_quantity[i] = 0;
          EEPROM.writeInt(20 + i * 2, 0);
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Memory cleared");
        delay(100);
      }
      if (digitalRead(calibr_button)) {   
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Calibration");
        break;
      }
    }
    while (1) {
      for (byte i = 0; i < coin_amount; i++) {
        lcd.setCursor(0, 1); lcd.print(coin_value[i]); 
        lcd.setCursor(13, 1); lcd.print(currency);      
        last_sens_signal = empty_signal;
        while (1) {
          sens_signal = analogRead(IRsens);                                    
          if (sens_signal > last_sens_signal) last_sens_signal = sens_signal;  
          if (sens_signal - empty_signal > 3) coin_flag = true;               
          if (coin_flag && (abs(sens_signal - empty_signal)) < 2) {            
            coin_signal[i] = last_sens_signal;                                 
            EEPROM.writeInt(i * 2, coin_signal[i]);
            coin_flag = false;
            break;
          }
        }
      }
      break;
    }
  }

  //
  for (byte i = 0; i < coin_amount; i++) {
    coin_signal[i] = EEPROM.readInt(i * 2);
    coin_quantity[i] = EEPROM.readInt(20 + i * 2);
    summ_money += coin_quantity[i] * coin_value[i];  
  }

  
  standby_timer = millis();  
}

void loop() {
  if (sleep_flag) { 
    delay(500);
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(L"Mhd Planet");
    lcd.setCursor(0, 1); lcd.print(summ_money);
    lcd.setCursor(13, 1); lcd.print(currency);
    empty_signal = analogRead(IRsens);
    sleep_flag = false;
  }

  // 
  last_sens_signal = empty_signal;
  while (1) {
    sens_signal = analogRead(IRsens); 
    if (sens_signal > last_sens_signal) last_sens_signal = sens_signal;
    if (sens_signal - empty_signal > 3) coin_flag = true;
    if (coin_flag && (abs(sens_signal - empty_signal)) < 2) {
      recogn_flag = false;  // флажок ошибки, пока что не используется
      // в общем нашли максимум для пролетевшей монетки, записали в last_sens_signal
      // далее начинаем сравнивать со значениями для монет, хранящимися в памяти
      for (byte i = 0; i < coin_amount; i++) {
        int delta = abs(last_sens_signal - coin_signal[i]);   
        if (delta < 30) {  
          summ_money += coin_value[i];  
          lcd.setCursor(0, 1); lcd.print(summ_money);
          coin_quantity[i]++;
          recogn_flag = true;
          break;
        }
      }
      coin_flag = false;
      standby_timer = millis(); 
      break;
    }

    // 
    if (millis() - standby_timer > stb_time) {
      good_night();
      break;
    }

    // 
    while (!digitalRead(button)) {
      if (millis() - standby_timer > 2000) {
        lcd.clear();

        // 
        for (byte i = 0; i < coin_amount; i++) {
          lcd.setCursor(i * 3, 0); lcd.print((int)coin_value[i]);
          lcd.setCursor(i * 3, 1); lcd.print(coin_quantity[i]);
        }
      }
    }
  }
}

// функция сна
void good_night() {
  //
  for (byte i = 0; i < coin_amount; i++) {
    EEPROM.updateInt(20 + i * 2, coin_quantity[i]);
  }
  sleep_flag = true;
  // 
  digitalWrite(disp_power, 0);
  digitalWrite(LEDpin, 0);
  digitalWrite(IRpin, 0);
  delay(100);
  // и вот теперь спать
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

//
void wake_up() {
  // возвращаем питание на дисплей и датчик
  digitalWrite(disp_power, 1);
  digitalWrite(LEDpin, 1);
  digitalWrite(IRpin, 1);
  standby_timer = millis(); 
}
