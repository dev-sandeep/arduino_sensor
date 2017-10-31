#include <ArduinoHttpClient.h>

/* include library for Wifi */
#include <SoftwareSerial.h>
#include <Wire.h>
/**
 * arduino RX pin=2  arduino TX pin=3  CH_PD - 3.3V
 * connect the arduino RX pin to esp8266 module TX pin   -  connect the arduino TX pin to esp8266 module RX pin
 */
SoftwareSerial espSerial(2, 3);

/**
 * Declarations for WIFI Module
 */
String apiKey = "7OF96TFNWAAZEMGT";     // replace with your channel's thingspeak WRITE API key
String ssid="Wolfpack";    // Wifi network SSID
String password ="Uwz181##";  // Wifi network password
boolean DEBUG=true;

/*
   Declrataios for IR Sensor
*/
#define irLedPin 4          // IR Led on this pin
#define irSensorPin 7       // IR sensor on this pin

boolean isDoorOpen = false;
int irValue;
int irTempVal = 0;

int doorLedPin =  60;//not used
/**
   Declaration for Ultrasound Sensor
*/
const int trigPin = 9;
const int echoPin = 10;

long duration;
int distance;

/**
   Declaration for ldr sensor
*/
unsigned int AnalogValue;

/**
   Declration for noise sensor
*/
int noiseSensorPin = A1;//Microphone Sensor Pin on analog 4
int tempSoundVal = 0;
int noiseSensorValDiff = 0;
int noiseSensorValue;

/* variables to save the sensor value */
int irSensorVal;
int ultraSensorVal;
int ldrSensorVal;
int noiseSensorVal;

/* hold temp values */
int tempIrSensorValue = 0;
int tempUltraSoundVal = 0;
int tempLdrSensorValue = 0;
int tempNoiseSensorValue = 0;

int customCtr = 250;//calls once in 5 sec approx
int sendDataCtr = 0;

/* hold the previously sent data */
int prevIrSensorValue = 0;
int prevUltraSoundVal = 0;
int prevLdrSensorValue = 0;
int prevNoiseSensorValue = 0;

/**
 * responsible for showing the response
 */
 void showResponse(int waitTime){
    long t=millis();
    char c;
    while (t+waitTime>millis()){
      if (espSerial.available()){
        c=espSerial.read();
        if (DEBUG) Serial.print(c);
      }
    }            
}

void setup()
{
  /* Setup for IR Sensor */
  pinMode(irSensorPin, INPUT);
  pinMode(irLedPin, OUTPUT);
  pinMode(doorLedPin, OUTPUT);

  digitalWrite(doorLedPin, 0);  
  
  /* Setup for Ultrasonic */
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  Serial.begin(9600);
  // prints title with ending line break
  Serial.println("Program Starting & settingup Wifi...");
  // wait for the long string to be sent
  delay(100);

  /**
   * WIFI setup starts from here
   */

  /**
   * enable software serial
   * Your esp8266 module's speed is probably at 115200. 
   * For this reason the first time set the speed to 115200 or to your esp8266 configured speed 
   * and upload. Then change to 9600 and upload again
   */
   espSerial.begin(115200);

   /* set esp8266 as client */
   espSerial.println("AT+CWMODE=1");
   showResponse(1000);

   /* connecting to the wifi */
   espSerial.println("AT+CWJAP=\""+ssid+"\",\""+password+"\"");  // set your home router SSID and password
   showResponse(5000);

   if (DEBUG)  Serial.println("Setup completed");
}

void loop()
{
  irSensor();
  untraSound();
  ldrSensor();
  noiseSensor();
  sendData();
}

/**
   Function to control the Noise sensor
   LOUD - Detected a sound
   QUIET - No noise
*/
void noiseSensor() {
  // read the value from the sensor:
  noiseSensorValue = analogRead(noiseSensorPin);

  //Serial.println(noiseSensorValue);
  
  noiseSensorValDiff = noiseSensorValue - tempSoundVal;
  tempSoundVal = noiseSensorValue;
  if (noiseSensorValDiff < 0)
    noiseSensorValDiff = noiseSensorValDiff * (-1);

  //if the val diff is more than 10, its a noise
  if (noiseSensorValDiff > 20) {
    //Serial.println("1");//LOUD
    setNoiseSensorValue(1);
  } else {
    //Serial.println("0");//QUIET
    setNoiseSensorValue(0);
  }
}

/**
   Function to control the LDR
   LIGHT - tubelight is on
   DARK - tubelight is off
*/
void ldrSensor() {
  AnalogValue = analogRead(A0);
  if (AnalogValue < 100) {
    //Serial.println("1");//Light
    setLdrSensorValue(1);
    
  } else {
    //Serial.println("0");//Dark
    setLdrSensorValue(0);
    
  }
}

/*
   Function for controlling the Ultrasound
   and get the distance
*/
void untraSound() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2;

  // Prints the distance on the Serial Monitor
  //Serial.print("DISTANCE: ");

  //get the difference between the present and previous values
  //if the difference is more than 1000 IGNORE
  int diff = tempUltraSoundVal - distance;
  if(diff < 0)
    diff *= -1;
  
  if(diff > 1000){
    //ignore this value
    ultraSensorVal += tempUltraSoundVal; 
    //Serial.println(tempUltraSoundVal);
  }else{
    ultraSensorVal += distance;
    /* save the present value temporarily */
    tempUltraSoundVal = distance;
    Serial.println(distance);
  }
}

/**
   Function for controlling the IR proximity sensor
   OPEN - The door is open
   CLOSE - The door is close
*/
void irSensor() {
  irValue = digitalRead(irSensorPin); //display the results
  if (irValue != irTempVal && irValue == 1 && isDoorOpen == false) {
    isDoorOpen = true;
    irTempVal = irValue;
    //Serial.println("1");
    setIrSensorValue(1);
    /* De-Activate the door led */
    digitalWrite(doorLedPin, 1);
    
  } else if (irValue != irTempVal && irValue == 1 && isDoorOpen == true) {
    isDoorOpen = false;
    irTempVal = irValue;
    //Serial.println("0");
    setIrSensorValue(0);
    /* De-Activate the door led */
    digitalWrite(doorLedPin, 0);
    
  }
  else {
    if (isDoorOpen == true) {
      //Serial.println("1");
      setIrSensorValue(1);
      /* Activate the door led */
      digitalWrite(doorLedPin, 1);
    }
    else {
      //Serial.println("0");
      setIrSensorValue(0);
      /* De-Activate the door led */
      digitalWrite(doorLedPin, 0);
    }
    irTempVal = irValue;
  }
  delay(10); //wait for the string to be sent
}

/**
 *  set the IR sensor value 
 */
void setIrSensorValue(int val){
  if(tempLdrSensorValue == 1){
    irSensorVal = 1; 
  }else{
    irSensorVal = val;
  }
  tempLdrSensorValue = val;
}

/**
 *  set the LDR sensor value 
 */
void setLdrSensorValue(int val){
  if(tempIrSensorValue == 1){
    ldrSensorVal = 1; 
  }else{
    ldrSensorVal = val;
  }
  tempIrSensorValue = val;
}

/**
 *  set the Noise sensor value 
 */
void setNoiseSensorValue(int val){
  if(tempNoiseSensorValue == 1){
    noiseSensorVal = 1; 
  }else{
    noiseSensorVal = val;
  }
  tempNoiseSensorValue = val;
}

/**
 * responsible for getting the present time
 */
 void getPresentTime(){
    /* make the TCP connection */
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    /* server's IP */
    cmd += "www.sandeepg.co.in";
    /* server's port number */
    cmd += "\",80";
    /* runing the command */
    espSerial.println(cmd);

    if(espSerial.find("Error")){
      if (DEBUG) Serial.println("AT+CIPSTART error while getting the time");
      return false;
    }

     /**
     * createing the GET API string
     */
     String getStr = "GET /api/?op=get_time";

     /* set the GET API string length + 20(anything more than the actual length of the string) */
     cmd = "AT+CIPSEND=";
     cmd += String(getStr.length()+20);

     /* running the command */
     espSerial.println(cmd);
     if (DEBUG)  Serial.println(cmd);
     /* runing it twice as it is failing due to strange reason if i send only once */
     espSerial.println(getStr);
     showResponse(1000);
     
     /* printing the response */
     espSerial.print("+IPD,12:");
     delay(1500);
     String inData = espSerial.readString();
     Serial.write(espSerial.read());
     Serial.println(inData);
 }

/**
 * responsible for calling the GET API to save the data
 */
 void callAPI(int irSensorVal, int ldrSensorVal, int noiseSensorVal){
    /* De-Activate the door led in case it is ON */
    digitalWrite(doorLedPin, 0);
    /* make the TCP connection */
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    /* server's IP */
    cmd += "184.106.153.149";
    /* server's port number */
    cmd += "\",80";
    /* runing the command */
    espSerial.println(cmd);
    if (DEBUG) Serial.println(cmd);
    if(espSerial.find("Error")){
      if (DEBUG) Serial.println("AT+CIPSTART error");
      return false;
    }

    /**
     * createing the GET API string
     */
      String getStr = "GET /update?api_key=";
      //String getStr = "GET ?api_key=";
      getStr += apiKey;
      /* setting up the sensor data in the API call string */      
      getStr +="&field1=";
      getStr += String(irSensorVal);
      getStr +="&field2=";
      getStr += String(ldrSensorVal);
      getStr +="&field3=";
      getStr += String(noiseSensorVal);
      getStr += "\r\n\r\n";

      /* set the GET API string length + 20(anything more than the actual length of the string) */
      cmd = "AT+CIPSEND=";
      cmd += String(getStr.length()+20);

      /* running the command */
      espSerial.println(cmd);
      if (DEBUG)  Serial.println(cmd);
      /* runing it twice as it is failing due to strange reason if i send only once */
      espSerial.println(getStr);
      showResponse(1000);
      if (DEBUG)  Serial.println(getStr);

      delay(100);

      /* printing the response */
      espSerial.print("+IPD,12:");
      delay(1500);
      String inData = espSerial.readString();
      Serial.write(espSerial.read());


      /* final Confirmation */
      if(espSerial.find(">")){
        espSerial.print(getStr);
        //if (DEBUG)  Serial.print(getStr);
      }
      else{
        espSerial.println("AT+CIPCLOSE");
        // alert user
        if (DEBUG)   Serial.println("AT+CIPCLOSE");
        return false;
      }
 }

 /**
  * responsible for gathering the data
  */
 void sendData(){
  /* check if its been called 200 times since the last API call */
  if(sendDataCtr++ < customCtr){
    return;
  }
  
  /**
   * check if any of the data got changed from the previous value
   */
  if(irSensorVal == prevIrSensorValue && ldrSensorVal == prevLdrSensorValue && noiseSensorVal == prevNoiseSensorValue){
    Serial.println("All values are same as the last one...");
    return;
  }

  /* set the data in the prevData */
  prevIrSensorValue = irSensorVal;
  prevLdrSensorValue = ldrSensorVal;
  prevNoiseSensorValue = noiseSensorVal;

  /* its once in 200 proceed! */
  Serial.println("--------------------------SEND DATA----------------------------------");
  sendDataCtr = 0;
  
  //getting the data
  Serial.println("IR Sensor: ");
  Serial.println(irSensorVal);
  Serial.println("LDR Sensor: ");
  Serial.println(ldrSensorVal);
  Serial.println("NOISE Sensor: ");
  Serial.println(noiseSensorVal);
  
  /* hitting the URL */
  //callAPI(irSensorVal,ldrSensorVal,noiseSensorVal);
  getPresentTime();
 }

 /**
  * clear all the values
  */
  void clearAllValues(){
    irSensorVal = 0;
    ultraSensorVal = 0;
    ldrSensorVal = 0;
    noiseSensorVal = 0;
  }

