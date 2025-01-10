#include <Arduino.h>

/*************************************************************
   Medição de nível do rio com o Espressif - ESP32 DEVKIT V1
   10.01.2025

  Blynk library is licensed under MIT license
 *************************************************************
  Blynk.Edgent implements:
  - Blynk.Inject - Dynamic WiFi credentials provisioning
  - Blynk.Air    - Over The Air firmware updates
  - Device state indication using a physical LED
  - Credentials reset using a physical Button
 *************************************************************/

#define BLYNK_TEMPLATE_ID "TMPLaM1hhk7O"
#define BLYNK_TEMPLATE_NAME "Pivo"
#define BLYNK_FIRMWARE_VERSION        "0.1.1"
//#define BLYNK_PRINT Serial
#define APP_DEBUG                  // desligar
#define USE_ESP32_DEV_MODULE
#include "BlynkEdgent.h"

// ----------------------------------- Watchdog ------------------------------------------
#include "soc/rtc_wdt.h"
#define WDT_TIMEOUT   120000                      // XXX miliseconds WDT

//const int IN3 = 33;         // sensor de niveis digital
//const int IN4 = 25;
//const int IN5 = 26;     
const int nivelSensor = 34;   // sensor de nivel analógico
int nivel = 0; 

void Main2(){
  rtc_wdt_feed();                                 // reseta o temporizador do Watchdog
  sensorNivel();
}

void sensorNivel() {  
    //   rotina para uso de sensores digitais
    //int A; int B; int C;
    //if (digitalRead(IN3) == HIGH) {A = 33;} else {A = 0;}        
    //if (digitalRead(IN4) == HIGH) {B = 33;} else {B = 0;}        
    //if (digitalRead(IN5) == HIGH) {C = 34;} else {C = 0;}
    //int nivel = A + B + C; 

    //   rotina para uso de sensor de nivel analógico
    nivel = analogRead (nivelSensor);
    nivel = map(nivel, 700, 3000, 100, 0);

    // imprime o resultado e envia ao servidor
      Serial.print("Nível é de : ");
      Serial.print(nivel);
      Serial.println("cm");   
      Blynk.virtualWrite(V0, nivel);  
  }

void setup(){
  // configuração do RTC Watchdog
  rtc_wdt_protect_off();                  //Disable RTC WDT write protection
  rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_RTC);
  rtc_wdt_set_time (RTC_WDT_STAGE0, WDT_TIMEOUT);
  rtc_wdt_enable();                       //Start the RTC WDT timer
  rtc_wdt_protect_on();                   //Enable RTC WDT write protection

  Serial.begin(115200);
  delay(100);
  edgentTimer.setInterval(1000L, Main2);                     // rotina se repete a cada XXXXL (milisegundos)
  BlynkEdgent.begin();
}

void loop(){
  BlynkEdgent.run();
}
