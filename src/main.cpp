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
#define BLYNK_TEMPLATE_ID             "TMPLaM1hhk7O"
#define BLYNK_TEMPLATE_NAME           "Pivo"
#define BLYNK_FIRMWARE_VERSION        "0.1.3"

//#define BLYNK_PRINT Serial
//#define APP_DEBUG
// #define USE_ESP32_DEV_MODULE
// #define a custom board in Settings.h (LED no pino GPIO 2)
#include "BlynkEdgent.h"

int currentSec;
int currentMin;
int currentHour;
int currentDay;
int currentMonth;
int currentYear;

int var = 0;

Preferences  preferences;                      // biblioteca para armazenamento de dados
unsigned int  counterRST;                      // contador de reset's
uint32_t servicoIoTState;                      // recebe a informação de BlynkState::get();
String     StrStateBlynk;                      // string de BlynkState para mostrar em um display
bool    sendBlynk = true;                      // usado como flag para envio ao servidor

// ----------------------------------- SETUP Watchdog ------------------------------------------
#include "soc/rtc_wdt.h"
#define  WDT_TIMEOUT        120000                          // XXX miliseconds WDT

// Converte razões do reset para string
const char *resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_UNKNOWN:   return "UNKNOWN RESET";
    case ESP_RST_POWERON:   return "POWER ON RESET";        //Power on or RST pin toggled
    case ESP_RST_EXT:       return "EXTERN PIN RESET";      //External pin - not applicable for ESP32
    case ESP_RST_SW:        return "SOFTWARE REBOOT";       //esp_restart()
    case ESP_RST_PANIC:     return "CRASH RESET";           //Exception/panic
    case ESP_RST_INT_WDT:   return "INTERRUPT WATCHDOG";    //Interrupt watchdog (software or hardware)
    case ESP_RST_TASK_WDT:  return "TASK WATCHDOG";         //Task watchdog
    case ESP_RST_WDT:       return "RTC WATCHDOG";          //Other watchdog
    case ESP_RST_DEEPSLEEP: return "SLEEP RESET";           //Reset after exiting deep sleep mode
    case ESP_RST_BROWNOUT:  return "BROWNOUT RESET";        //Brownout reset (software or hardware)
    case ESP_RST_SDIO:      return "RESET OVER SDIO";       //Reset over SDIO
    default:                return "";
  }
}

void PrintResetReason(void) {
  esp_reset_reason_t r = esp_reset_reason();
  Serial.printf("Reset reason:     %i - %s\r\n\r\n", r, resetReasonName(r));
}
// ----------------------------------- Fim Watchdog ----------------------------------------

//const int IN3 = 33;         // sensor de niveis digital
//const int IN4 = 25;
//const int IN5 = 26;     
const int nivelSensor = 34;   // sensor de nivel analógico
int nivel = 0; 

//-------------------------------------  NTP Server time ----------------------------------------------
const char* ntpServer = "br.pool.ntp.org";      // "pool.ntp.org"; "a.st1.ntp.br";
const long  gmtOffset_sec = -10800;             // -14400; Fuso horário em segundos (-03h = -10800 seg)
const int   daylightOffset_sec = 0;             // ajuste em segundos do horario de verão

//-------------------------------------  Sensor interno de temepratura --------------------------------
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

// ------ Protótipo de funções ------
void sensorNivel(void);
void NTPserverTime(void);
void sendLogReset(void);
void colorLED(void);

void Main2(){
  unsigned long tempo_start = millis();      // usado no final para imprimir o tempo de execução dessa rotina

    // habilitar para teste de Sofware Reboot ou RTC Watchdog
  //if (currentSec == 59){ESP.restart();}
  //if (currentSec == 59){int i = WDT_TIMEOUT/1000;
  //   while(1){Serial.print("Watchdog vai atuar em... "); Serial.println (i);delay(980);i = i - 1;}}
  
  if ((currentHour == 6) && (currentMin == 0) && (currentSec < 2)){
    preferences.begin  ("my-app", false);              // inicia 
    preferences.putUInt("counterRST", 0);              // grava em Preferences/My-app/counterRST, counterRST
    counterRST = preferences.getUInt("counterRST", 0); // Le da NVS
    preferences.end();
    Serial.println("A data e hora foram recalibradas no relógio interno, e o contador de RESETs foi zerado!");
    Blynk.virtualWrite(V45, currentDay, "/", currentMonth, " ", currentHour, ":", currentMin, " Auto calibrado o Relógio");
  }

  rtc_wdt_feed();                                 // reseta o temporizador do Watchdog;
  NTPserverTime();                                // busca e envia a data/hora
  sensorNivel();                                  // busca e envia a informação de nível

    // ------ Integrações com o app ------
servicoIoTState = BlynkState::get();    // requisita estado da biblioteca BlynkEdgent.h
   switch (servicoIoTState) {
      case 0: StrStateBlynk = "WAIT CONFIG"  ; break;
      case 1: StrStateBlynk = "CONFIGURING"  ; break;
      case 2: StrStateBlynk = "NET Connect"  ; break;
      case 3: StrStateBlynk = "CLOUD Connect"; break;
      case 4: StrStateBlynk = "RUNNING..."   ; break;
      case 5: StrStateBlynk = "UPGRADE OTA"  ; break;
      case 6: StrStateBlynk = "STATION Mode" ; break;
      case 7: StrStateBlynk = "RESET Config" ; break;
      case 8: StrStateBlynk = "ERROR !"      ; break;
    }

  // ------ Coleta e envio do nivel de RF ------
  long rssi = WiFi.RSSI();
  Serial.print("RF Signal Level: ");
  Serial.println(rssi);                           // Escreve o indicador de nível de sinal Wi-Fi
  Blynk.virtualWrite(V3, rssi);                   // Envia ao Blynk informação RF Signal Level


  Serial.printf("Período (ms) do Main2: %u\n", (millis() - tempo_start)); // cálculo do tempu utilizado até aqui
  Serial.println("------------------------------------------------------");  
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
      currentYear  = timeinfo.tm_year + 1900;
      currentMonth = timeinfo.tm_mon + 1;
      currentDay   = timeinfo.tm_mday;

      currentHour  = timeinfo.tm_hour;
      currentMin   = timeinfo.tm_min;
      currentSec   = timeinfo.tm_sec;

      sendLogReset();                                               // manda o log uma vez

      char RTC_Time[64];                                            //Cria uma string formatada da estrutura "timeinfo"
      sprintf(RTC_Time, "%02d.%02d.%04d  -  %02d:%02d:%02d", currentDay, currentMonth, currentYear, currentHour, currentMin, currentSec);
      Serial.print("Data/hora do sistema:  ");
      Serial.println(RTC_Time);
      Blynk.virtualWrite(V1, RTC_Time);                             // envia ao Blynk a informação de data, hora e minuto do RTC
    
      int temp=((temprature_sens_read() - 32) / 1.8)-7;             // -7 Viamão,   -31 Restinga Seca 
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.println(" C");
      Blynk.virtualWrite(V2, temp);
    }
}

void sendLogReset(){
  // envia razao do reset para o servidor
  if ((servicoIoTState==4) && (sendBlynk)){
    Serial.print("               BLYNK:  RODANDO COM SUCESSO!"); // delay(100);
    esp_reset_reason_t r = esp_reset_reason();
    Serial.printf("\r\nReset reason %i - %s\r\n", r, resetReasonName(r));
    Blynk.virtualWrite(V45, currentDay, "/", currentMonth, " ", currentHour, ":", currentMin, "",resetReasonName(r), " ",counterRST);
    Blynk.virtualWrite(V53, counterRST);                        // envia para tela do app
    Blynk.syncVirtual (V40, V69, V89);                          // sincroniza datastream de agendamentos
    delay(500);
    // se reiniciar por (1) POWER ON RESET
    if (r == 1){
      //Blynk.logEvent("falha_de_energia", String("Teste - Falha de Energia!"));
      Blynk.logEvent("falha_de_energia");                 // registra o evento falha_de_energia no servidor
      }
    sendBlynk = false;
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
  PrintResetReason();                     // imiprime a razão do último reset

  // ------   Inicia e cria espaco na memoria NVS - namespace:my-app    ------
  preferences.begin("my-app", false);   // Note: Namespace name is limited to 15 chars.
  //preferences.clear();                // remove all preferences under the opened namespace
  //preferences.remove("counterRST");   // or remove the counter key only
  counterRST = preferences.getUInt("counterRST", 0); // Le da NVS, se counterRST nao existir retorna o valor 0
  counterRST++;                         // incrementa a cada restart
  //Serial.printf("Quantidade de RESETs: %u\n", counterRST);
  preferences.putUInt("counterRST", counterRST);  // grava em Preferences/My-app/counterRST, counterRST
  preferences.end();
  delay(50);
  Serial.printf("Quantidade de RESETs: %u\n", counterRST);
  sendBlynk = true;

  edgentTimer.setInterval(1000L, Main2);     // rotina se repete a cada XXXXL (milisegundos)
  edgentTimer.setInterval(1000L, colorLED);  // rotina se repete a cada XXXXL (milisegundos)
  BlynkEdgent.begin();
  
}

// Rotina de testes para troca de cores dos Widget's
void colorLED(void){
  var = var + 1;
  if (var >0 && var<2) {
    //#define BLYNK_GREEN     "#23C48E"
    Blynk.setProperty(V10, "color", "#00FF6E"); // verde
    Blynk.setProperty(V11, "color", "#FF0000"); // vermelho
  } 
  
  if (var >2 && var<4) {
    //#define BLYNK_BLUE      "#04C0F8"
    Blynk.setProperty(V10, "color", "#2865F4"); // azul
    Blynk.setProperty(V11, "color", "#F7CB46"); // laranja
  } 
  
  if (var >4 && var<6) {
    //#define BLYNK_YELLOW    "#ED9D00"
    Blynk.setProperty(V10, "color", "#F7CB46"); // laranja
    Blynk.setProperty(V11, "color", "#2865F4"); // azul
  } 

  if (var >6 && var<8) {
    //#define BLYNK_RED       "#D3435C"
    Blynk.setProperty(V10, "color", "#FF0000"); // vermelho
    Blynk.setProperty(V11, "color", "#00FF6E"); // verde
  } 

  if (var >= 8) {var = 0;}
}

void loop(){
  BlynkEdgent.run(); 
}
