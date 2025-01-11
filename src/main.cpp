#include <Arduino.h>

/*************************************************************
   Medição de nível do rio com o Espressif - ESP32 DEVKIT V1
   11.01.2025

  Blynk library is licensed under MIT license
 *************************************************************
  Blynk.Edgent implements:
  - Blynk.Inject - Dynamic WiFi credentials provisioning
  - Blynk.Air    - Over The Air firmware updates
  - Device state indication using a physical LED
  - Credentials reset using a physical Button
 *************************************************************/
#include "time.h" 

//#define BLYNK_TEMPLATE_ID "TMPL2Yf6rOHsV"
//#define BLYNK_TEMPLATE_NAME "Pivo Teste"
#define BLYNK_TEMPLATE_ID "TMPLaM1hhk7O"
#define BLYNK_TEMPLATE_NAME "Pivo"
#define BLYNK_FIRMWARE_VERSION        "0.1.2"

//#define BLYNK_PRINT Serial
//#define APP_DEBUG
// #define USE_ESP32_DEV_MODULE
// #define a custom board in Settings.h (LED no pino GPIO 2)
#include "BlynkEdgent.h"

// ----------------------------------- Watchdog ------------------------------------------
#include "soc/rtc_wdt.h"
#define WDT_TIMEOUT   120000                      // xxxxxx milisegundos (max: 120000)

//const int IN3 = 33;         // sensor de niveis digital
//const int IN4 = 25;
//const int IN5 = 26;     
const int nivelSensor = 34;   // sensor de nivel analógico
int nivel = 0; 

//-------------------------------------  NTP Server time ----------------------------------------------
const char* ntpServer = "br.pool.ntp.org";      // "pool.ntp.org"; "a.st1.ntp.br";
const long  gmtOffset_sec = -10800;             // -14400; Fuso horário em segundos (-03h = -10800 seg)
const int   daylightOffset_sec = 0;             // ajuste em segundos do horario de verão

// ------ protótipo de funções ------
void sensorNivel(void);
void NTPserverTime(void);

void Main2(){
  rtc_wdt_feed();                                 // reseta o temporizador do Watchdog;
  NTPserverTime();                                // busca e envia a data/hora
  sensorNivel();                                  // busca e envia a informação de nível
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
    nivel = map(nivel, 912, 3648, 100, 0);        // comentar essa linha para calibrar (ADC vai de 0 a 4095)

    //   imprime o resultado e envia ao servidor
    Serial.print("Nível é de : ");
    Serial.print(nivel);
    Serial.println("cm");   
    Blynk.virtualWrite(V0, nivel);  
  }

void NTPserverTime(){          // Horário recebido da internet
  struct tm timeinfo;
     //  "%u, %d.%m.%Y %H:%M:%S"  = 1, 17.08.2021 14:33:33
     //   %u = dia da semana em decimal, range 1 a 7, Segunda = 1.
     // https://www.ibm.com/docs/en/z-workload-scheduler/9.5.0?topic=troubleshooting-date-time-format-reference-strftime 
  if(!getLocalTime(&timeinfo)){                                    // testa conexão com a internet
    Serial.println("Sincronizando com o servidor NTP...");
    Blynk.virtualWrite(V1, "Sincronizando...");                    // envia ao Blynk
    } else {                                                       // quando conectar segue...
      int ye = timeinfo.tm_year + 1900;
      int mo = timeinfo.tm_mon + 1;
      int da = timeinfo.tm_mday;

      int ho = timeinfo.tm_hour;
      int mi = timeinfo.tm_min;
      int se = timeinfo.tm_sec;

      char RTC_Time[64];                                            //Cria uma string formatada da estrutura "timeinfo"
      sprintf(RTC_Time, "%02d.%02d.%04d  -  %02d:%02d:%02d", da, mo, ye, ho, mi, se);
      Serial.print("Data/hora do sistema:  ");
      Serial.println(RTC_Time);
      Blynk.virtualWrite(V1, RTC_Time);                             // envia ao Blynk a informação de data, hora e minuto do RTC
    }
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
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   // inicia e busca as infos de data e hora (NTP)
  edgentTimer.setInterval(1000L, Main2);                      // rotina se repete a cada XXXXL (milisegundos)
  BlynkEdgent.begin();
}

void loop(){
  BlynkEdgent.run();
}
