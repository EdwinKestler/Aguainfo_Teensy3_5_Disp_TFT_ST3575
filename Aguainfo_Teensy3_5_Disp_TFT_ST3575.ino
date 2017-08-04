/***************************************************
  This is the first version for an automatic waterDispenser Logger
  Hardware:
  Microcontroller: 
    Teensy 3.5 https://www.pjrc.com/store/teensy35.html
  Sensors:
    Reed Water Flow sensor 1/8" http://a.co/1OLH5OE
    Time Of FLigth VL53L0X distance Sensor https://www.pololu.com/product/2490
  Actuators:
    1.8TFT 128x160 Color Screen   https://www.aliexpress.com/item/Consumer-Electronics-Shop-Free-shipping-1-8-Serial-128X160-SPI-TFT-LCD-Module-Display-PCB-Adapter/32602828489.html
    10A 250VAC SRD-05VDC-SL-C Relay 
    12V RGB LED STRIP  
    12V Normally Closed Solenoid Valve http://a.co/acy6ti3
  PowerSupply:
    110-220VAC Dual output 5V 1A - 12V 1A power suppply http://a.co/2KBeenJ
        
  Written by Edwin Kestler For Aguainfo Guatemala.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// This Teensy3 native optimized version requires specific pins

#define TFT_SCLK 13  // SCLK can also use pin 14
#define TFT_MOSI 11  // MOSI can also use pin 7
#define TFT_CS   10  // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define TFT_DC    9  //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define TFT_RST   8  // RST can use any pin
#define SD_CS     4  // CS for SD card, can use any pin

#include <Adafruit_GFX.h>    // Core graphics library
#include <ST7735_t3.h> // Hardware-specific library
#include <SPI.h>
#include "valve.h"
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

int distance, OldDistance;
boolean Bottle, OldBottle;
volatile float waterFlow;
volatile float oldwaterflow;
//-------------------------------------------------From strings Demo--------------------------------------------
String text ="";
int TextColor = 0;
const int NUMBER_OF_ELEMENTS = 11;
const int MAX_SIZE = 70;

char descriptions [NUMBER_OF_ELEMENTS] [MAX_SIZE] = { 
 { " ¡Luchemos por un país lleno de agua limpia! " }, 
 { " Una botella plástica menos en ríos y mares, ¡Gracias! " }, 
 { " ¡Protejamos las fuentes de agua para las futuras generaciones! " }, 
 { " La magia de este planeta está en el agua " }, 
 { " El agua es vida, no la desperdiciemos " }, 
 { " ¡Beber agua te reanima y te regala sonrisas! " }, 
 { " Proteger el agua es proteger a la humanidad " }, 
 { " Cuidar el agua es cuidar tu salud " }, 
 { " Tomar agua nos da vida. Tomar conciencia nos dará agua " }, 
 { " Beber agua es bueno para una piel más sana " },
 { " Tú eres lo que bebes " }, 
 };


// Option 1: use any pins but a little slower
ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//ST7735_t3 tft = ST7735_t3(cs, dc, rst);
//float p = 3.1415926;

#define HIGH_SPEED
//#define HIGH_ACCURACY

unsigned long lastSenseMillis, lastFSenseMillis;
unsigned long UPDATELASERM = 500UL; //Variable que define el tiempo a trancurrir despues de inicializado el reloj para enviar el primer mensake de MQTT en microsegundos (10*1000UL)= 10segundos

Valve portA(6);
int BotellaServidas;
int S = 0;

void setup(void) {
  
  pinMode(SD_CS, INPUT_PULLUP);  // don't touch the SD card
  Serial.begin(115200);
  Serial.print("hello!");

  //-----------------------------------------inciando la pantalla-------------------------------------------------------------------------------//
  Serial.println(F("inicializando ST7735"));
  Serial.println("ST7735 Test!"); 
   // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);
  Serial.println("init");
  Serial.println(F("Finalizing Setup")); //enviamos un mensaje de depuracion 
  tft.setRotation(1);
  tft.println(F("Finalizing Setup")); //enviamos un mensaje de depuracion 
  delay(250);
  Wire.begin();
  sensor.init();
  sensor.setTimeout(500);
  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  #if defined LONG_RANGE
  // lower the return signal rate limit (default is 0.25 MCPS)
  sensor.setSignalRateLimit(0.1);
  // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  #endif
  #if defined HIGH_SPEED
  // reduce timing budget to 20 ms (default is about 33 ms)
  sensor.setMeasurementTimingBudget(50000);
  #elif defined HIGH_ACCURACY
  // increase timing budget to 200 ms
  sensor.setMeasurementTimingBudget(200000);
  #endif
  // ---------------------------------------------Finalizando el Setup
  uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  tft.println(F("Ready to start")); //enviamos un mensaje de depuracion 
  time = millis() - time;
  // ---------------------------------------------Setting up Water Flow Sensor
  waterFlow = 0;
  attachInterrupt(2, pulse, RISING);  //DIGITAL Pin 2: Interrupt 0
  Serial.println(time, DEC);
  delay(500);
}

void pulse(){   //measure the quantity of square wave
  waterFlow += 1.0/1350;
  //DisplayStrings();  
}


void loop() {
  if(millis() - lastSenseMillis > UPDATELASERM) {
    SenseDistance();
    lastSenseMillis = millis(); //Actulizar la ultima hora de envio
  }
   
  CheckBottle();
  CheckWaterFlow();
  IOValve();
     
}

void CheckBottle(){
   if (distance != OldDistance){
    if (distance < 100){
      Bottle = 1;
    }
    else{
      Bottle = 0;
    }
    OldDistance = distance;
  }  
}

void CheckWaterFlow(){
  if(millis() - lastFSenseMillis > UPDATELASERM) {
    Serial.print(F("WaterinLtrs: "));
    Serial.println(waterFlow);
    Serial.print(F("difference: "));
    Serial.println(waterFlow - oldwaterflow);
    lastFSenseMillis = millis(); //Actulizar la ultima hora de envio      
  }
  
  if (waterFlow - oldwaterflow >= 0.1){
    DisplayStrings();    
    oldwaterflow = waterFlow;
    ScreenUpdate();
  }
}

void IOValve(){
  if (Bottle != OldBottle){
    if (Bottle !=0){
      portA.Update(LOW);
      }else {
        portA.Update(HIGH);
        ScreenUpdate();
        BotellaServidas ++;
      }
   }
   OldBottle = Bottle;  
}


/*void FillBottle(){
  if (oldwaterflow - waterFlow < 1 ){
    portA.Update(LOW);
    LitrosAhorrados ++;
    oldwaterflow = waterFlow ;
    }else{
      portA.Update(HIGH);
      BotellaServidas ++;
    }
    OldBottle = Bottle;
}        */
 
void DisplayStrings(){ 
  tft.fillScreen(ST7735_BLACK);
  if (TextColor == 0){
    tft.setTextColor(ST7735_BLUE, ST7735_BLACK);  // White on black
    TextColor = 1;
  }else{
    tft.setTextColor(ST7735_GREEN, ST7735_BLACK);  // White on black
    TextColor = 0;
  }
  tft.setTextWrap(false);  // Don't wrap text to next line
  tft.setFont();
  tft.setTextSize(5);  // large letters
  tft.setRotation(1); // horizontal display
  text = descriptions [S];
  //Serial.println (descriptions [i]);
  const int width = 5;  // width of the marquee display (in characters)
  // Loop once through the string
  for (unsigned int offset = 0; offset < text.length(); offset++){
    // Construct the string to display for this iteration
    String t = "";
    for (int i = 0; i < width; i++)
    t += text.charAt((offset + i) % text.length()-1);
    // Print  the string for this iteration
    tft.setCursor(0, tft.height()/2-10);  // display will be halfway down screen
    tft.print(t);
    // Short delay so the text doesn't move too fast
    delay(200);
  }
  if(S < NUMBER_OF_ELEMENTS){
    S++;
  }else{
    S=0;
  }
}

void ScreenUpdate(){
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(30, 10);
  tft.setTextColor(ST7735_GREEN);
  tft.setFont(&FreeSerifBold18pt7b);
  tft.setCursor(50, 50);
  tft.setTextSize(1);
  tft.println(BotellaServidas);
  tft.setCursor(50, 100);
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(1);
  tft.print(waterFlow,1);
  tft.println(F("L"));
}

void SenseDistance(){
  distance = sensor.readRangeSingleMillimeters();
  //Serial.print(distance);
  if (sensor.timeoutOccurred()) {
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(10, 35);
    tft.setTextColor(ST7735_GREEN);
    tft.setTextSize(2);
    Serial.print(" TIMEOUT");
    tft.println("TIMEOUT");
  }
  Serial.println(F(""));
}

