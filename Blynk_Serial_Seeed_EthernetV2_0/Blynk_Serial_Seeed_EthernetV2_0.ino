/**************************************************************
 * Blynk is a platform with iOS and Android apps to control
 * Arduino, Raspberry Pi and the likes over the Internet.
 * You can easily build graphic interfaces for all your
 * projects by simply dragging and dropping widgets.
 *
 *   Downloads, docs, tutorials: http://www.blynk.cc
 *   Blynk community:            http://community.blynk.cc
 *   Social networks:            http://www.fb.com/blynkapp
 *                               http://twitter.com/blynk_app
 *
 * Blynk library is licensed under MIT license
 *
 **************************************************************
 *
 * This sketch is for use with wirecast_serial_ole.pl
 *
 * Author: Ford Polia
 *
 **************************************************************/

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <SPI.h>
#include <EthernetV2_0.h>
#include <BlynkSimpleEthernetV2_0.h>
#include <BMDSDIControl.h>

#define W5200_CS  10
#define SDCARD_CS 4

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "e1c914f18a6041bc816598a7623a3da8";

#define PIN_LIVE_CAMERA V0

WidgetLED previewIndicator(V5);
WidgetLED liveIndicator(V6);

int currentCamera;
int liveCamera;
int previewCamera;
bool autoIris[4];
bool autoFocus[4];
float audioLevels[2];

// Blackmagic Design SDI control shield globals
const int                 shieldAddress = 0x6E;
BMD_SDITallyControl_I2C   sdiTallyControl(shieldAddress);
BMD_SDICameraControl_I2C  sdiCameraControl(shieldAddress);

void setup()
{
  Serial.begin(9600);

  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH); // Deselect the SD card

  Blynk.begin(auth);
  // You can also specify server.
  // For more options, see BoardsAndShields/Arduino_Ethernet_Manual example
  //Blynk.begin(auth, "blynk-cloud.com", 8442);
  //Blynk.begin(auth, IPAddress(192,168,1,100), 8888);
  
  // Set up the BMD SDI control library
  sdiTallyControl.begin();
  sdiCameraControl.begin();

  // Enable both tally and control overrides
  sdiTallyControl.setOverride(true);
  sdiCameraControl.setOverride(true);
}

// This function tells Arduino what to do if there is a Widget
// which is requesting data for Virtual Pin (0)
BLYNK_READ(PIN_LIVE_CAMERA)
{
  Blynk.virtualWrite(PIN_LIVE_CAMERA, liveCamera);
  if (currentCamera == liveCamera){
    liveIndicator.on();
    previewIndicator.off();
  } else if (currentCamera == previewCamera){
    liveIndicator.off();
    previewIndicator.on();
  } else {
    liveIndicator.off();
    previewIndicator.off();
  }
}

BLYNK_WRITE(V1) //user selected camera 1
{
  if (param.asInt() == 1){
    currentCamera = 1;
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V4, 0);
  }
}

BLYNK_WRITE(V2) //User selected camera 2
{
  if (param.asInt() == 1){
    currentCamera = 2;
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V4, 0);
  }
}

BLYNK_WRITE(V3) //User selected camera 3
{
  if (param.asInt() == 1){
    currentCamera = 3;
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V4, 0);
  }
}

BLYNK_WRITE(V4) //User selected camera 4
{
  if (param.asInt() == 1){
    currentCamera = 4;
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 0);
  }
}

BLYNK_WRITE(V7) //User adjusted focus
{
  sdiCameraControl.writeCommandFixed16(
      currentCamera,          // Destination Camera
      0,                      // Category:       Lens
      0,                      // Param:          Focus
      0,                      // Operation:      Assign Value
      (param.asFloat()/100.0) // Value
    );
}

BLYNK_WRITE(V8) {
  switch (param.asInt())
  {
    case 1: { // User selected autofocus off
      autoFocus[currentCamera] = false;
      break;
    }
    case 2: { // User selected autofocus on
      autoFocus[currentCamera] = true;
      break;
    }
    default: {
      autoFocus[currentCamera] = false;
    }
  }
}

BLYNK_WRITE(V9) //User adjusted aperture
{
  sdiCameraControl.writeCommandFixed16(
      currentCamera,          // Destination Camera
      0,                      // Category:       Lens
      3,                      // Param:          Aperture (Normalised)
      0,                      // Operation:      Assign Value
      (param.asFloat()/100.0) // Value
    );
}

BLYNK_WRITE(V10) {
  switch (param.asInt())
  {
    case 1: { // User selected auto iris off
      autoIris[currentCamera-1] = false;
      break;
    }
    case 2: { // User selected auto iris on
      autoIris[currentCamera-1] = true;
      break;
    }
    default: {
      autoIris[currentCamera-1] = false;
    }
  }
}

BLYNK_WRITE(V11) //User adjusted zoom
{
  sdiCameraControl.writeCommandFixed16(
      currentCamera,          // Destination Camera
      0,                      // Category:       Lens
      8,                      // Param:          Zoom (Normalised)
      0,                      // Operation:      Assign Value
      (param.asFloat()/100.0) // Value
    );
}

BLYNK_WRITE(V12) //User adjusted exposure
{
  sdiCameraControl.writeCommandFixed16(
      currentCamera,          // Destination Camera
      1,                      // Category:       Video
      5,                      // Param:          Exposure (Normalised)
      0,                      // Operation:      Assign Value
      (param.asFloat()/1000.0)// Value
    );
}

BLYNK_WRITE(V13) //User adjusted audio ch 0
{
  audioLevels[0] = param.asFloat()/100.0;
  sdiCameraControl.writeCommandFixed16(
      currentCamera,          // Destination Camera
      2,                      // Category:       Audio
      8,                      // Param:          Input Levels
      0,                      // Operation:      Assign Value
      audioLevels             // Values
    );
}

BLYNK_WRITE(V14) //User adjusted audio ch 1
{
  audioLevels[1] = param.asFloat()/100.0;
  sdiCameraControl.writeCommandFixed16(
      currentCamera,          // Destination Camera
      2,                      // Category:       Audio
      8,                      // Param:          Input Levels
      0,                      // Operation:      Assign Value
      audioLevels             // Values
    );
}

void loop()
{
  int inInt;
  while(Serial.available()<=0) { //Waits for data to be available on serial port
   Blynk.run();
  }
  char inChar = Serial.read();
  if (inChar == 'C'){
    while (Serial.available()<=0){ //Waits for data to be available on serial port
      Blynk.run();
    }
    inInt = Serial.parseInt();
    while (Serial.available()<=0){ //Waits for data to be available on serial port
      Blynk.run();
    }
    inChar = Serial.read();
    if (inChar == 'P'){
      previewCamera = inInt;
      if (autoFocus[inInt - 1] == true){
        sdiCameraControl.writeCommandVoid(
          inInt,  // Destination:    Camera #
          0,      // Category:       Lens
          1       // Param:          Auto Focus
        );
      }
      if (autoIris[inInt - 1] == true){
        sdiCameraControl.writeCommandVoid(
          inInt,  // Destination:    Camera #
          0,      // Category:       Lens
          5       // Param:          Auto Iris
        );
      }
      sdiTallyControl.setCameraTally(    
        inInt, 
        false, 
        true   
      );
    } else if (inChar == 'L'){
      liveCamera = inInt;
      sdiTallyControl.setCameraTally(    
        inInt,                                                 // Camera Number
        true,                                                  // Program Tally
        false                                                  // Preview Tally
      );
    } else {
      sdiTallyControl.setCameraTally(    
        1, 
        false, 
        false
      );
    }
  }
}

