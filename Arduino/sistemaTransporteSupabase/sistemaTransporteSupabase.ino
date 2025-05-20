/** -----------------------------------------
  * MFRC522      NodeMCU
  * Reader       Esp8266
  * Pin          Pin    
  * ---------------------
  * SDA(SS)      D4*
  * SCK          D5   
  * MOSI         D7
  * MISO         D6
  * RST          D3*
  * NC(IRQ)      notused
  * 3.3V         3V
  * GND          GND
  * --------------------------------------------------------------------------
  */
#include "RfidDictionaryView.h"


#define Debug_Serial_Mon
void WIFI_Connect(const char* wifi_id, const char* wifi_password);


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "INFINITUM0E20";
const char* password = "9MnrNtY4rc";

const char* serverName = "https://dlkdjvlfmjeiyfszghrg.supabase.co/rest/v1/rpc/check_saldo";
const char* apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImRsa2RqdmxmbWplaXlmc3pnaHJnIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDY2NjAzNzIsImV4cCI6MjA2MjIzNjM3Mn0.hBo6U1TwD_FOozBUJ9ZcN-LOH9VHYDpEUeWJsSjg5_0";


// Outputs
#define GreenLED D8
#define RedLED D2
#define BUZZER_PIN D1 

// PIN MODO RECARGA *A0* a *3V3*


// NFC define
#define START_BLOCK 18
RfidDictionaryView rfidDict(D4, D3, START_BLOCK);
bool tagSelected = false;


// printf-style function for serial output
void printfSerial(const char* fmt, ...);


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(20000);  // waits for up to 20s in "read" functions

  delay(1000);  //to wait the serial monitor to start (in PlatformIO)

  // Outputs
  pinMode(GreenLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WIFI_Connect(ssid, password);
}


void loop() {

  // Read NFC
  byte tagId[4] = { 0, 0, 0, 0 };

  if (!tagSelected) {
    Serial.println();
    Serial.println("APPROACH a Mifare tag. Waiting...");

    do {
      // returns true if a Mifare tag is detected
      tagSelected = rfidDict.detectTag(tagId);
      delay(5);
    } while (!tagSelected);

    printfSerial("- TAG DETECTED, ID = %02X %02X %02X %02X \n", tagId[0], tagId[1], tagId[2], tagId[3]);
    printfSerial("  space available for dictionary: %d bytes.\n\n", rfidDict.getMaxSpaceInTag());
  }

  String id = rfidDict.get("id");  // read id;
  Serial.println("Tarjera ID: " + id);

  // Desconectar tarjeta
  rfidDict.disconnectTag(true);
  tagSelected = false;
  Serial.println("Please move the tag away in 4 secs");  //2s below, and 2s in the end of the function

  // Verificar conexion
  if (WiFi.status() != WL_CONNECTED) {
    WIFI_Connect(ssid, password);
  }

  bool aceptado = sendToSupabase(id);


  if (aceptado) {  // Si paso
                   //
    Serial.println("ACEPTADO se cobro la tarifa correctamente");

    //output
    digitalWrite(GreenLED, HIGH);
    tone(BUZZER_PIN, 2960, 300);
    delay(600);
    digitalWrite(GreenLED, LOW);

  } else {  // No paso
    Serial.println("RECHAZADO no se pudo cobrar a la tarjeta");

    //output
    digitalWrite(RedLED, HIGH);
    tone(BUZZER_PIN, 1500, 300);
    delay(400);
    tone(BUZZER_PIN, 1500, 300);
    delay(300);
    digitalWrite(RedLED, LOW);
  }


  // if (cargoFlag) {
  //   if (WiFi.status() != WL_CONNECTED) {WIFI_Connect(ssid, password);}

  //   Serial.println("Enviando datos a sheets");
  //   Data_to_Sheets(id, saldoSTR);  // envia los datos a sheets
  //   cargoFlag = false;
  // } else {
  //   delay(2000);
  // }



  while (Serial.available() > 0) {  // clear "garbage" input from serial
    Serial.read();
  }
  Serial.println("Finished operation!");
  Serial.println();
  // delay(1000);
}

void printfSerial(const char* fmt, ...) {
  char buf[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.print(buf);
}

void WIFI_Connect(const char* wifi_id, const char* wifi_password) {
  digitalWrite(GreenLED, HIGH);
  digitalWrite(RedLED, HIGH);

#ifdef Debug_Serial_Mon
  Serial.print("connecting to ");
  Serial.println(wifi_id);
#endif

  // WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_id, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

#ifdef Debug_Serial_Mon
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  digitalWrite(GreenLED, LOW);
  digitalWrite(RedLED, LOW);
}


bool sendToSupabase(const String id) {
  WiFiClientSecure client;
  client.setInsecure();  // Use only if you don't validate the SSL certificate

  HTTPClient http;
  http.begin(client, serverName);

  // Set headers
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", apiKey);
  http.addHeader("Authorization", "Bearer " + String(apiKey));

  // JSON data
  String jsonData = "{\"p_id\": \"" + id + "\"}";

  // Send the POST request
  Serial.println("Sending HTTP POST");
  int httpResponseCode = http.POST(jsonData);

  // Check the response
  String response = http.getString();
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response code: " + String(httpResponseCode));
    Serial.println("Response body: " + response);
  } else {
    Serial.println("Error on sending POST: " + String(httpResponseCode));
  }

  if (response == "true") {
    return true;

  } else if (response == "false") {
    return false;
  }

  // Close connection
  http.end();
}
