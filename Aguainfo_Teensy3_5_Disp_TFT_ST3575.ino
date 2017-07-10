/***************************************************
  This is an example sketch for the Adafruit 1.8" SPI display.

This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
The 1.8" TFT shield
  ----> https://www.adafruit.com/product/802
The 1.44" TFT breakout
  ----> https://www.adafruit.com/product/2088
as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// This Teensy3 native optimized version requires specific pins
//
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

#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;
int distance, OldDistance;
boolean Bottle, OldBottle;
volatile double waterFlow;

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

unsigned long lastSenseMillis, StartMillis;
unsigned long UPDATELASERM = 500UL; //Variable que define el tiempo a trancurrir despues de inicializado el reloj para enviar el primer mensake de MQTT en microsegundos (10*1000UL)= 10segundos

Valve portA(6);
int BotellaServidas, LitrosAhorrados;

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
  waterFlow += 1.0 / 5880.0;
}

void loop() {
  if(millis() - lastSenseMillis > UPDATELASERM) {
    SenseDistance();
    lastSenseMillis = millis(); //Actulizar la ultima hora de envio
  }
  
  if (distance != OldDistance){
    if (distance < 100){
      Bottle = 1;
    }
    else{
      Bottle = 0;
    }
    OldDistance = distance;
  }
  
  ScreenUpdate();
}

void ScreenUpdate(){
  if (Bottle != OldBottle){
    if (Bottle == 1){
      portA.Update();
      LitrosAhorrados ++;
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(50, 10);
      tft.setTextColor(ST7735_GREEN);
      tft.setTextSize(2);
      Serial.print(F("LLenando"));
      //tft.println("LLENANDO");
      tft.setTextSize(2);
      tft.println("Litros");
      tft.setCursor(30, 35);
      tft.println("Ahorrados");
      tft.setCursor(35, 60);
      tft.setTextSize(4);
      tft.println(waterFlow);
    }
    else{
      portA.Update();
      BotellaServidas ++;
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(30, 10);
      tft.setTextColor(ST7735_GREEN);
      tft.setTextSize(2);
      Serial.print(F("ESPERANDO"));
      //tft.println("ESPERANDO");
      tft.setTextSize(2);
      tft.println("Botellas");
      tft.setCursor(30, 35);
      tft.println("Servidas");
      tft.setCursor(60, 60);
      tft.setTextSize(4);
      tft.println(BotellaServidas);
    }
    OldBottle = Bottle;
  }
}

void SenseDistance(){
  distance = sensor.readRangeSingleMillimeters();
  Serial.print(distance);
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

