
/* Turtle Vision Power Box
 * Written by:
 * Thomas Spellman <thomas@thosmos.com>
 * Copyright: 2013, Thomas Spellman (http://turtlevision.co)
 * License: This code is distributed under the GPL license: http://www.gnu.org/licenses/gpl.html
 * 1.0.0: Turtle Vision Power Box v. 1.0.0
 * 1.0.4: changing json format to nested arrays
 * 1.0.6: defines for two box types
 */

//#include "tv.h"

#define TVONE 1
//#define TVTWO 1

#define BOX_SMALL 1
#define BOX_MEDIUM 2

#define VOLT_102 1
#define VOLT_200 2

#if TVONE

#define BOX_TYPE BOX_MEDIUM
#define MAX_VOLT VOLT_102
#define BOX_NAME "TV One"
#define BOX_ID 0

#elif TVTWO

#define BOX_TYPE BOX_SMALL
#define MAX_VOLT VOLT_200
#define BOX_NAME "TV Two"
#define BOX_ID 1

#endif

#define VERSION "Turtle Vision Power Box v. 1.0.6"

#define AVG_CYCLES 100.0
#define BLINK_INTERVAL 250
#define DISPLAY_INTERVAL 1000
#define AUTODATA_INTERVAL 500
#define VOLT_INTERVAL 10

#define ENABLE_RELAY 0
#define ENABLE_STREAM 0
#define ENABLE_INDICATOR 1

#if ENABLE_INDICATOR
#include "LPD8806.h"
#include "SPI.h"
#define INDICATOR_INTERVAL 250
#define NUM_INDICATOR_LEDS 36
#define PIN_INDICATOR_DATA 2
#define PIN_INDICATOR_CLK 3
#endif

#if ENABLE_RELAY
#define VOLT_TEST_INTERVAL 1000
#define VOLT_CUTOFF 28.0
#define VOLT_RECOVER 27.0
#define PIN_RELAY 11
#endif

#if ENABLE_STREAM 
#define NUM_STREAM_SAMPLES 16
#endif

#define PIN_VOLTS A6
#define PIN_LED 13

#if TVONE

#define ENABLE_BATTERY 1
#define NUM_AMP_SENSORS 10
#define NUM_OUTPUTS 3
#define NUM_INPUTS 6
#define PIN_BATTERY A11
#define PIN_OUTPUTS { A7, A8, A10 }
#define PIN_INPUTS { A0, A1, A2, A3, A4, A5 }
#define PIN_AMPS { A0, A1, A2, A3, A4, A5, A7, A8, A10, A11 }
#define SENSOR_TYPES { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }
//#define OFFSETS { -24, -24, -24, -24, -24, -24, -24, -24, -24, -24 }

#elif TVTWO

#define ENABLE_BATTERY 1
#define NUM_AMP_SENSORS 9
#define NUM_OUTPUTS 2
#define NUM_INPUTS 6
#define PIN_BATTERY A10
#define PIN_OUTPUTS { A7, A8 }
#define PIN_INPUTS { A0, A1, A2, A3, A4, A5 }
#define PIN_AMPS { A0, A1, A2, A3, A4, A5, A7, A8, A10 }
#define SENSOR_TYPES { 50, 50, 50, 50, 50, 50, 50, 50, 50 }
//#define OFFSETS { -24, -24, -24, -24, -24, -24, -24, -24, -24 }

#endif


int pinAmps[NUM_AMP_SENSORS] = PIN_AMPS;
int sensorType[NUM_AMP_SENSORS] = SENSOR_TYPES;
int pinOuts[NUM_OUTPUTS] = PIN_OUTPUTS;
int pinIns[NUM_INPUTS] = PIN_INPUTS;
//int sensorOffset[NUM_AMP_SENSORS] = OFFSETS;

int ampsRaw[NUM_AMP_SENSORS] = {0};

#if ENABLE_STREAM
int streamVolts[NUM_STREAM_SAMPLES] = {0};
int streamAmps[NUM_AMP_SENSORS][NUM_STREAM_SAMPLES] = {0};
#endif

float volts = 0.0;
float amps[NUM_AMP_SENSORS] = {0.0};
float watts[NUM_AMP_SENSORS] = {0.0};
float energy[NUM_AMP_SENSORS] = {0.0};
//float energyTotal[NUM_AMP_SENSORS] = {0.0};
float wattsIn = 0.0;
float wattsOut = 0.0;
float totalIn = 0.0;
float totalOut = 0.0;
float energyIn = 0.0;
float energyOut = 0.0;
float batteryWatts = 0.0;
float batteryIn = 0.0;
float batteryOut = 0.0;
float batteryTotal = 0.0;

//typedef unsigned long ulong;
unsigned long time = 0;
unsigned long lastTime = 0;
unsigned long lastVolt = 0;
unsigned long lastVoltTest = 0;
unsigned long lastDisplay = 0;
unsigned long lastBlink = 0;
unsigned long lastEnergy = 0;
unsigned long lastDoData = 0;
unsigned long lastIndicator = 0;
unsigned long lastAutoData = 0;

#if ENABLE_RELAY
boolean isRelayOn = false;
#endif

boolean disableBikes = false;
boolean enableVoltTest = true;
boolean enableAutoDisplay = false;
boolean enableRawMode = false;
boolean enableStream  = false;
boolean enableAutoData = false;

unsigned int voltAdc = 0;
float voltAdcAvg = 0;

unsigned int sampleCount = 0;

boolean isBlinking = false;
boolean initted = false;

#if ENABLE_INDICATOR
LPD8806 strip = LPD8806(NUM_INDICATOR_LEDS, PIN_INDICATOR_DATA, PIN_INDICATOR_CLK);
#endif

void setup(){
  
  pinMode(PIN_LED, OUTPUT);  
  pinMode(PIN_VOLTS, INPUT); // voltage ADC

  for(int i = 0; i < NUM_AMP_SENSORS; i++){
    pinMode(pinAmps[i], INPUT);
  }

#if ENABLE_RELAY
  pinMode(PIN_RELAY, OUTPUT); 
#endif

  Serial.begin(115200);
  Serial1.begin(115200); // Yun UART

#if ENABLE_INDICATOR
  pinMode(PIN_INDICATOR_DATA, OUTPUT);
  pinMode(PIN_INDICATOR_CLK, OUTPUT);
  
  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
#endif

}

void loop(){
  
  //rainbowCycle(0);  // make it go through the cycle fairly fast
  
  time = millis();

  if(!enableStream && time - lastVolt > VOLT_INTERVAL){
    lastVolt = time;
    doEnergy();
  }

#if ENABLE_STREAM  
  if(enableStream){
    doStream();
  }
#endif

#if ENABLE_RELAY
  if(time - lastVoltTest > VOLT_TEST_INTERVAL){
    lastVoltTest = time;
    doVoltTest();
  }
#endif

  if(enableAutoDisplay && time - lastDisplay > DISPLAY_INTERVAL){
    lastDisplay = time;

    doDisplay();
    
  }
  
  if(enableAutoData && time - lastAutoData > AUTODATA_INTERVAL){
    lastAutoData = time;
    //doData();
    doData1();
  }

  if(time - lastTime > BLINK_INTERVAL){
    lastTime = time;
    doBlink();
  } 

#if ENABLE_INDICATOR
  //doIndicator();
  doIndChase();
#endif

//  if(time - lastIndicator > INDICATOR_INTERVAL){
//   lastIndicator = time;
//    doIndicator(); 
//  }
  
   if (Serial.available() > 0) {
         int in = Serial.read();
         doSerial(in);
   }
   if (Serial1.available() > 0) {
         int in = Serial1.read();
         doSerial1(in);
   }
}

void pr(char* str, int dst){
  
}

void prln(char* str, int dst){

}

void doSerial(int in){
      // get incoming byte:
    switch(in){
      case 'a':
        //Serial.print("Enable Auto Data Mode:");
        //Serial.println(enableAutoData);
        enableAutoData = true;
        break;
      case 'b':
        //Serial.print("Enable Auto Data Mode:");
        //Serial.println(enableAutoData);
        enableAutoData = false;
        break;
      case 'd':
        doData();
        doData1();
        break;
      case 'e': 
        enableAutoDisplay = !enableAutoDisplay;
        break;
      case 'h':
        enableRawMode = !enableRawMode;
        Serial.print("Enable Raw Mode:");
        Serial.println(enableRawMode);
        break;
      case 'p':
        doDisplay();
        break;
        
#if ENABLE_RELAY        
      case 'r':
        doRelay(!isRelayOn);
        break;
#endif
#if ENABLE_STREAM
      case 's':
        // stream data
        enableStream = !enableStream;
        Serial.print("Enable Data Stream:");
        Serial.println(enableStream);
        break;
#endif        
#if ENABLE_RELAY        
      case 't':
        enableVoltTest = !enableVoltTest;
        Serial.print("Enable Volt Test:");
        Serial.println(enableVoltTest);
        break;
#endif

      case 'v':
        // do a bunch of volts samples
        break;
      case 'x':
        resetEnergy();
        totalIn = 0.0;
        totalOut = 0.0;
        break;
      case 'z': // version
        Serial.println(VERSION);
        break;
      default:
        break;
    }
}

int zcount = 0;

void doSerial1(int in){
      // get incoming byte:
    
 //only handle if preceeded by 5 zs (zzzzza)
  if(in == 'z')
    zcount++;
  else {
    if(zcount < 5)
      zcount = 0;
    else {
      switch(in){
        case 'a':
          //Serial.print("Enable Auto Data Mode:");
          //Serial.println(enableAutoData);
          enableAutoData = true;
          break;
        case 'b':
          //Serial.print("Enable Auto Data Mode:");
          //Serial.println(enableAutoData);
          enableAutoData = false;
          break;
        case 'd':
          //doData();
          doData1();
          break;
        default:
          break;
      }
      zcount = 0;
    }
  }
}

#if ENABLE_STREAM
void doStream(){
  int i, j;
  //Serial.write(0xFFFA);
  Serial.print("{");
  Serial.print("\"timeA\":");
  Serial.print(millis());
  Serial.print(",\"volts\":[");
  for( i = 0; i < NUM_STREAM_SAMPLES; i++){
    streamVolts[i] = analogRead(PIN_VOLTS);
    //int sent = Serial.write(streamVolts[i]);
    Serial.print(streamVolts[i]);
    if(i < NUM_STREAM_SAMPLES - 1)
      Serial.print(",");
    //Serial.print("sent: ");
    //Serial.println(sent);
  }
  Serial.print("],\"amps\":[");
  for(j = 0; j < NUM_AMP_SENSORS; j++){
    Serial.print("[");
    for(i = 0; i < NUM_STREAM_SAMPLES; i++){
      streamAmps[j][i] = analogRead(pinAmps[j]);
      //Serial.write(streamAmps[j][i]);
      Serial.print(streamAmps[j][i]);
      if(i < NUM_STREAM_SAMPLES - 1)
        Serial.print(",");
    }
    Serial.print("]");
    if(j < NUM_AMP_SENSORS - 1)
      Serial.print(",");
  }
  Serial.print("]");
#if ENABLE_RELAY  
  Serial.print(",\"relay\":");
  Serial.print(isRelayOn);
#endif  
  Serial.print(",\"timeB\":");
  Serial.print(millis());
  Serial.println("}");
  //Serial.write(0xFFFB);
}
#endif

void doEnergy(){
  sampleCount++; 

  float temp = 0.0;
  int tempI = 0;
  int i = 0;

  //measure volts
    voltAdc = analogRead(PIN_VOLTS);
    temp = adc2volts((float)voltAdc);
    //Serial.println(temp);
    //volts = temp;
    volts = averageF(volts, temp);
  //}

  float timeDiff = time - lastEnergy;
  float timeDiffSecs = timeDiff / 1000.0;
  
  wattsIn = 0.0;
  wattsOut = 0.0;

  // measure amps and calc energy
  for(i = 0; i < NUM_AMP_SENSORS; i++){
        ampsRaw[i] = analogRead(pinAmps[i]);
        
        //ampsRaw[i] = ampsRaw[i] + sensorOffset[i];
        //tempI = ampsRaw[i] + sensorOffset[i];
        tempI = ampsRaw[i];
        if(tempI > 504 && tempI < 518)
          tempI = 511;

        temp = adc2amps(tempI, sensorType[i]);
        //amps[i] = temp;
        amps[i] = averageF(amps[i], temp);
      //}
    
    //calc watts and energy
    watts[i] = volts * amps[i];
    float wattsecs = watts[i] * timeDiffSecs;
    float watthrs = wattsecs / 3600;
    energy[i] += wattsecs; // watt secs
    //energyTotal[i] += watthrs;  // watt hours
    
    // if this is the battery pin, then handle differently
    if(PIN_BATTERY == pinAmps[i]){
      batteryWatts = watts[i];
      if(watts[i] < 0.0){
        batteryOut += wattsecs;
      } else {
        batteryIn += wattsecs;
      }
      batteryTotal += watthrs;
    }
    else {
      if(watts[i] < 0.0){
        wattsOut += watts[i];
        energyOut += wattsecs;
        totalOut += watthrs;
      } else {
        wattsIn += watts[i];
        energyIn += wattsecs;
        totalIn += watthrs;
      }
    }    
//    if(i < 6){
//      wattsIn += watts[i];
//      energyIn += wattsecs;
//      totalIn += watthrs;
//    }
//    else if(i == 6){
//      batteryWatts = watts[i];
//      batteryWattSecs += wattsecs;
//      batteryWattHrs += watthrs;
//    }
//    else if (i == 7){
//      wattsOut = watts[i];
//      energyOut += wattsecs;
//      totalOut += watthrs;
//    }

  }
  lastEnergy = time;
}

#if ENABLE_RELAY
void doVoltTest(){
  if(!enableVoltTest)
    return;
    
  //float volts = adc2volts(voltAdcAvg);
  if(volts > VOLT_CUTOFF)
  {
    //digitalWrite(PIN_RELAY, HIGH);
    //disableBikes = true;
    //isRelayOn = true;
    doRelay(true);
  }
  else if(isRelayOn && volts < VOLT_RECOVER)
  {
    //digitalWrite(PIN_RELAY, LOW);
    //disableBikes = false;
    doRelay(false);
    //isRelayOn = false;
  }
}

void doRelay(boolean doRelay){
 if(doRelay){
   digitalWrite(PIN_RELAY, HIGH);
   isRelayOn = true;
 } else {
   digitalWrite(PIN_RELAY, LOW);
   isRelayOn = false;
 }
}
#endif

float averageF(float avg, float val){
  if(avg == 0)
    avg = val;
  return (val + (avg * (AVG_CYCLES - 1))) / AVG_CYCLES;
}

// valid JSON
void doData(){
  Serial.println("{");
  Serial.print("\"BOX_ID\": ");
  Serial.print(BOX_ID);
  Serial.println(",");
  Serial.print("\"VOLTS\": ");
  Serial.print(volts, 2);
  Serial.println(",");
  Serial.println("\"CURRENT\": [");
  
  for(int i = 0; i < NUM_AMP_SENSORS; i++){
    if(pinAmps[i] == PIN_BATTERY)
      continue;
    Serial.println("  {");
    Serial.print("    \"AMPS\": ");
    Serial.print(amps[i], 2);
    Serial.println(",");
    Serial.print("    \"WATTS\": ");
    Serial.print(watts[i], 2);
    Serial.println(",");
    Serial.print("    \"ENERGY\": ");
    Serial.println(energy[i], 2);
    Serial.print("  }");
    if(i < (NUM_AMP_SENSORS - 2)){
      Serial.println(",");
    }
    else {
      Serial.println("");
    }
  }
  Serial.println("],");
  Serial.print("\"WATTS_IN\": ");
  Serial.print(wattsIn, 0);
  Serial.println(",");
  Serial.print("\"WATTS_OUT\": ");
  Serial.print(wattsOut, 0);
  Serial.println(",");
  Serial.print("\"TOTAL_IN\": ");
  Serial.print(totalIn, 2);
  Serial.println(",");
  Serial.print("\"TOTAL_OUT\": ");
  Serial.print(totalOut, 2);
  Serial.println(",");
  Serial.print("\"BATTERY_WATTS\": ");
  Serial.print(batteryWatts, 2);
  Serial.println(",");
  Serial.print("\"BATTERY_IN\": ");
  Serial.print(batteryIn, 0);
  Serial.println(",");
  Serial.print("\"BATTERY_OUT\": ");
  Serial.print(batteryOut, 0);
  Serial.println(",");
  Serial.print("\"SAMPLES\": ");
  Serial.print(sampleCount);
  Serial.println(",");
  Serial.print("\"REL_TIME\": ");
  Serial.print(time - lastDoData);
  Serial.println(",");
  lastDoData = time;
  Serial.print("\"ARDUINO_TIME\": ");
  Serial.print(time);
#if ENABLE_RELAY
  if(isRelayOn)
    Serial.println(",");
    Serial.println("\"RELAY\": \"ON\"");
#else
    Serial.println("");
#endif    
  Serial.println("}");
}

// valid JSON
void doData1(){
  Serial1.println("{");
  Serial1.print("\"BOX_ID\": ");
  Serial1.print(BOX_ID);
  Serial1.println(",");
  Serial1.print("\"VOLTS\": ");
  Serial1.print(volts, 2);
  Serial1.println(",");
  Serial1.println("\"CURRENT\": [");
  
  for(int i = 0; i < NUM_AMP_SENSORS; i++){
    if(pinAmps[i] == PIN_BATTERY)
      continue;
    Serial1.println("  {");
    Serial1.print("    \"AMPS\": ");
    Serial1.print(amps[i], 2);
    Serial1.println(",");
    Serial1.print("    \"WATTS\": ");
    Serial1.print(watts[i], 2);
    Serial1.println(",");
    Serial1.print("    \"ENERGY\": ");
    Serial1.println(energy[i], 2);
    Serial1.print("  }");
    if(i < (NUM_AMP_SENSORS - 2)){
      Serial1.println(",");
    }
    else {
      Serial1.println("");
    }
  }
  Serial1.println("],");
  Serial1.print("\"WATTS_IN\": ");
  Serial1.print(wattsIn, 0);
  Serial1.println(",");
  Serial1.print("\"WATTS_OUT\": ");
  Serial1.print(wattsOut, 0);
  Serial1.println(",");
  Serial1.print("\"TOTAL_IN\": ");
  Serial1.print(totalIn, 2);
  Serial1.println(",");
  Serial1.print("\"TOTAL_OUT\": ");
  Serial1.print(totalOut, 2);
  Serial1.println(",");
  Serial1.print("\"BATTERY_WATTS\": ");
  Serial1.print(batteryWatts, 2);
  Serial1.println(",");
  Serial1.print("\"BATTERY_IN\": ");
  Serial1.print(batteryIn, 0);
  Serial1.println(",");
  Serial1.print("\"BATTERY_OUT\": ");
  Serial1.print(batteryOut, 0);
  Serial1.println(",");
  Serial1.print("\"SAMPLES\": ");
  Serial1.print(sampleCount);
  Serial1.println(",");
  Serial1.print("\"REL_TIME\": ");
  Serial1.print(time - lastDoData);
  Serial1.println(",");
  lastDoData = time;
  Serial1.print("\"ARDUINO_TIME\": ");
  Serial1.print(time);
#if ENABLE_RELAY
  if(isRelayOn)
    Serial1.println(",");
    Serial1.println("\"RELAY\": \"ON\"");
#else
    Serial1.println();
#endif    
  Serial1.println("}");
}

// valid JSON
void doDisplay(){
  Serial.print("{VOLTS: ");
  Serial.print(volts, 2);
  Serial.print(", RAW: ");
  Serial.print(voltAdc);

  for(int i = 0; i < NUM_AMP_SENSORS; i++){
    Serial.print(", ");
    Serial.print(i + 1);
    Serial.print(": ");
    if(enableRawMode)
      Serial.print(ampsRaw[i]);
    else
      Serial.print(watts[i], 0);
  }
  Serial.print(", IN: ");
  Serial.print(wattsIn, 0);
  Serial.print(", OUT: ");
  Serial.print(wattsOut, 0);
//  Serial.print(", NRG IN: ");
//  Serial.print(energyIn, 0);
//  Serial.print(", NRG OUT: ");
//  Serial.print(energyOut, 0);
  Serial.print(", BATTERY_WATTS: ");
  Serial.print(batteryWatts, 2);
  Serial.print(", BATTERY_IN: ");
  Serial.print(batteryIn, 0);
  Serial.print(", BATTERY_OUT: ");
  Serial.print(batteryOut, 0);
  Serial.print(", TOTAL_IN: ");
  Serial.print(totalIn, 2);
  Serial.print(", TOTAL_OUT: ");
  Serial.print(totalOut, 2);
#if ENABLE_RELAY
  if(isRelayOn)
  Serial.print(", RELAY: ON");
#endif  
  Serial.println("}");
  //resetEnergy();
}

void resetEnergy(){
  sampleCount = 0;
  for(int i = 0; i < NUM_AMP_SENSORS; i++){
    energy[i] = 0.0;
  }
  energyIn = 0.0;
  energyOut = 0.0;
//  batteryWattSecs = 0.0;
//  batteryWattHrs = 0.0;
}

void doBlink(){
  isBlinking = !isBlinking;
  digitalWrite(PIN_LED, isBlinking);
  //digitalWrite(PIN_INDICATOR_DATA, isBlinking);
  //digitalWrite(PIN_INDICATOR_CLK, isBlinking);
  //Serial.print("BLINK: ");
  //Serial.println(isBlinking);
}


static int volts2adc(float v){
 /* voltage calculations
 *
 * Vout = Vin * R2/(R1+R2), where R1 = 100k, R2 = 10K 
 * 30V * 10k/110k = 2.72V      // at ADC input, for a 36.3V max input range
 *
 * Val = Vout / 3.3V max * 1024 adc val max (2^10 = 1024 max vaue for a 10bit ADC)
 * 2.727/3.3 * 1024 = 846.196363636363636 
 */
//int volt30 = 846;

/* 24v
 * 24v * 10k/110k = 2.181818181818182
 * 2.1818/3.3 * 1024 = 677.024793388429752
 */
//int volt24 = 677;

//adc = v * 10/110/3.3 * 1024 == v * 28.209366391184573;
//return v * 28.209366391184573;

//adc = v * 10/110/5 * 1023 == v * 18.6;

// R1 = 156k, R2 = 4k
//adc = v * 4/160/5 * 1024 == v * 5.12
//return v * 5.12;

#if MAX_VOLT == VOLT_200
  //adc = v * 3.9/158.9/5 * 1024 == v * 5.02655758339072
  return v * 5.02655758339072;
#elif MAX_VOLT == VOLT_102
  //adc = v * 6.8/139.8/5 * 1024 == v * 9.96165951358976
  return v * 9.96165951358976;
#endif  


  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  //vals[i] = reads[i] * (5.0 / 1023.0);

}


float adc2volts(float adc){
  // v = adc * 110/10 * 3.3 / 1024 == adc * 0.03544921875;
  //return adc * 0.03544921875; // 36.3 / 1024 = 0.03544921875; 
  
  // v = adc * 160/4 * 5 / 1024 = adc * 0.1953125
  // v = adc * (155+3.9)/3.9 * 5 / 1024 = adc * 0.19894330929487
  
  //return adc * 0.1953125;
#if MAX_VOLT == VOLT_200
  // v = adc * (155+3.9)/3.9 * 5 / 1024 = adc * 0.19894330929487
  return adc * 0.19894330929487;
#elif MAX_VOLT == VOLT_102
  // v = adc * (133+6.8)/6.8 * 5 / 1023 = adc * 0.10048300845265
  return adc * 0.10048300845265;
#endif  
  //return (adc + 2) * 0.19913777978294;
  
}

//float adc2amps50(float adc){
//  float pinVolts = adc * 5 / 1023;
//  //float pinVolts = adc * 0.004887585532747;
//  return (pinVolts - 2.5) / 2.5 * 50;
//  //return (pinVolts - 2.5) * 0.008;
//}
//
//float adc2amps100(float adc){
//  float pinVolts = adc * 5 / 1023;
//  return (pinVolts - 2.5) / 2.5 * 100;
//  //return (pinVolts - 2.5) * 0.004;
//}

//converts adc value to adc pin volts
//float adc2pinVolts(float adc){
 // adc * 3.3 / 1024 == adc *  0.00322265625
 //return adc * 0.00322265625;
//}

// amp sensor conversion factors
// 0A == 512 adc == 2.5pinVolts // current sensor offset
// pV/A(100) = .04 pV/A (@5V)  (* 3.3V/5V = .0264 pV/A (@3.3V)) // sensor sensitivity (pV = adc input pin volts) 
// pV/A(50) = .02 pV/A (@5V)  // sensor sensitivity (pV = adc input pin volts) 
// adc/pV = 1023 adc / 5 pV = 204.6 adc/pV  // adc per pinVolt
// adc/A(50) = 204.6 adc/pV * 0.04 pV/A = 8.184 adc/A
// adc/A(100) = 204.6 adc/pV * 0.02 pV/A = 4.092 adc/A
// A(50)/adc = 1 A / 8.184 adc = 0.122189638318671 A/adc
// A100/adc = 1 A / 4.092 adc = 0.244379276637341 A/adc
//
//float adc2amps100(float adc){
//  // A/adc = 0.1220703125 A/adc
//  return (adc - 512) * 0.1220703125;
//}
//
//float adc2amps50(float adc){
//  // A/adc = 0.244379276637341 A/adc
//  return (adc - 512) * 0.244379276637341;
//}

float adc2amps(float adc, int sensorType){
  return (adc - 511) * (sensorType == 100 ? 0.244379276637341 : 0.1220703125);
}

float amps2adc(float amps, int sensorType){
  // adc/A = 8.192 adc/A
  return 511 + amps * (sensorType == 100 ? 4.092 : 8.192);
}


#if ENABLE_INDICATOR

#define NUM_INDICATOR_LEDS 36
#define VOLT_CUTOFF 28.0
#define VOLT_RECOVER 27.0

#define VOLT_BLINK 7.0
#define VOLT_LOW 24.0
//#define VOLT_MED 25.0
//#define VOLT_HIGH 28.0

#define IND_STATE_CHANGE_INTERVAL 500
#define IND_BLINK_INTERVAL 500

#define STATE_OFF 0
#define STATE_ON 1
#define STATE_BLINK_LOW 2
#define STATE_BLINK_HIGH 3
#define STATE_RAMP 4

int indVoltLevel = 0;
int indBlinkState = STATE_OFF;
int indState = STATE_OFF;
uint32_t indColor = 0; 
unsigned long lastIndBlink = 0;
unsigned long lastIndStateChange = 0;

void doIndicator(){
 
  // test for state change
  if(time - lastIndStateChange > IND_STATE_CHANGE_INTERVAL){
    lastIndStateChange = time;
    
    if(volts < VOLT_BLINK){
      indState = STATE_OFF;
    }
    else if(volts < VOLT_LOW){
      indState = STATE_BLINK_LOW;
    }
    else if(volts < VOLT_CUTOFF){
      indState = STATE_RAMP;
    }

#if ENABLE_RELAY
    if(isRelayOn){
      indState = STATE_BLINK_HIGH;
    }
#endif

  }
  
  if(indState == STATE_BLINK_LOW || indState == STATE_BLINK_HIGH){
    doIndBlink();
    doIndChase();
  } else if(indState == STATE_RAMP){
    doIndRamp();
    doIndChase();
  } else if(indState == STATE_OFF){
    doIndOff();
  }
  
//  if(indState == STATE_BLINK_LOW || indState == STATE_RAMP)
  
}

void doIndOff(){
  int i;
  // turn all pixels off:
  for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);
}

void doIndBlink(){
  
  if(time - lastIndBlink > IND_BLINK_INTERVAL){
    lastIndBlink = time;
    indBlinkState = !indBlinkState;
    
    int i;
    // turn all pixels off:
    for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);
  
    if(indState == STATE_BLINK_LOW && indBlinkState){
      
      for(i=0; i<strip.numPixels(); i++)
        strip.setPixelColor(i,strip.Color(127,0,0));
      
    }else if(indState == STATE_BLINK_HIGH && indBlinkState){
      
      for(i=0; i<strip.numPixels(); i++)
        strip.setPixelColor(i,strip.Color(127,127,127));
      
    }
    
    // no need to show strip as doChase will handle it
    //strip.show();
  }
  
}

#define IND_RAMP_INTERVAL 500
unsigned long lastIndRamp = 0;

#if ENABLE_BATTERY
void doIndBattery(){
  
    
  if(time - lastIndRamp < IND_RAMP_INTERVAL)
    return;
    
    lastIndRamp = time;
  
    int i, j;
  
    // turn all pixels off:
    for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);
  
    // set 1st 4 to red  
    for(i = 0; i < 4; i++)
    strip.setPixelColor(i, strip.Color(127, 0, 0));
    
    float level = (volts - VOLT_LOW);
    
    // 8 leds for each volt between 24 and 28
    int numPixels = level * 8;
    
    uint32_t color;
    if(level < 3.0){
      color = Wheel(level / 3.0 * 63.0); 
    } else {
     int colorAdd = ((level - 3.0) * 127.0);
     color = strip.Color(63, 63, colorAdd);
    }
    
    for(i = 0; i < numPixels + 4; i++){
//      if((i - 4) % 8 == 0)
//        strip.setPixelColor(i, strip.Color(127, 127, 127));
//      else
        strip.setPixelColor(i, color);
    }

    // no need to show strip as doChase will handle it
    //strip.show();  
}

#else
void doIndRamp(){
  
  if(time - lastIndRamp > IND_RAMP_INTERVAL){
    lastIndRamp = time;
  
    int i, j;
  
    // turn all pixels off:
    for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);
  
    // set 1st 4 to red  
    for(i = 0; i < 4; i++)
    strip.setPixelColor(i, strip.Color(127, 0, 0));
    
    float level = (volts - VOLT_LOW);
    
    // 8 leds for each volt between 24 and 28
    int numPixels = level * 8;
    
    uint32_t color;
    if(level < 3.0){
      color = Wheel(level / 3.0 * 63.0); 
    } else {
     int colorAdd = ((level - 3.0) * 127.0);
     color = strip.Color(63, 63, colorAdd);
    }
    
    for(i = 0; i < numPixels + 4; i++){
//      if((i - 4) % 8 == 0)
//        strip.setPixelColor(i, strip.Color(127, 127, 127));
//      else
        strip.setPixelColor(i, color);
    }

    // no need to show strip as doChase will handle it
    //strip.show();

   }

  
  //float color = level * 

//  for(i = 0; i < numPixels; i++){
//    if(i < 8){
//      // orange?
//      strip.setPixelColor(i + 4, strip.Color(127,63,0));
//    } else if(i < 16){
//      // gold?
//      strip.setPixelColor(i + 4, strip.Color(100,100,0));
//    } else if(i < 24){
//      //green
//      strip.setPixelColor(i + 4, strip.Color(0,127,0));
//    } else{
//      //white
//      strip.setPixelColor(i + 4, strip.Color(127,127,127));
//      for(j = 0; j < i + 4; i++){
//        uint32_t color = strip.getPixelColor(j);
//        // for each led of white, add more blue to all other colors
//         int blue = (i + 4 - 24) * 16;
//         uint32_t newColor = color + blue;
//         strip.setPixelColor(j, newColor);
//      }
//    }
//  }
  
}
#endif


#define CHASE_INTERVAL 50
#define DEBUG 0
float chasePos = 0.0;
int chasePosI = 0;
unsigned long lastChase;
int chaseCount = 0;

void doIndChase(){

  if(time - lastChase > CHASE_INTERVAL){
    chaseCount++;
    float wattsDiff = wattsIn + wattsOut;  
    if(DEBUG && chaseCount % 10 == 0){
      Serial.print("wattsDiff: ");
      Serial.print(wattsDiff);
    }

    //Serial.print(ampDiff);
    float rps = wattsDiff / 100.0;  // ramps per second
    
    float timeDiff = time - lastChase;  
    if(DEBUG && chaseCount % 10 == 0){
      Serial.print(", timeDiff: ");
      Serial.print(timeDiff);
    }
    lastChase = time;

    float posDiff = rps * timeDiff / ((float) NUM_INDICATOR_LEDS);
    if(DEBUG && chaseCount % 10 == 0){
      Serial.print(", posDiff: ");
      Serial.print(posDiff, 8);
    }    

    chasePos = chasePos + posDiff;
    if(DEBUG && chaseCount % 10 == 0){
      Serial.print(", chasePos: ");
      Serial.println(chasePos);
    }    

    if(chasePos > (NUM_INDICATOR_LEDS - 1))
      chasePos -= NUM_INDICATOR_LEDS;
    else if(chasePos < 0)
      chasePos += NUM_INDICATOR_LEDS;
    chasePosI = chasePos;
  
    uint32_t prevColor = strip.getPixelColor(chasePosI);
    strip.setPixelColor(chasePosI, strip.Color(127,127,127));
    strip.show();  
    strip.setPixelColor(chasePosI, prevColor);
  }
  
}

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 384; j++) {     // 3 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 384));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Fill the dots progressively along the strip.
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Chase one dot down the full strip.
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  // Start by turning all pixels off:
  for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c); // Set new pixel 'on'
    strip.show();              // Refresh LED states
    strip.setPixelColor(i, 0); // Erase pixel, but don't refresh!
    delay(wait);
  }

  strip.show(); // Refresh to turn off last pixel
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}

#endif  // ENABLE_INDICATOR


