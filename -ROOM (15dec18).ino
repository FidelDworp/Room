// ROOM.ino = Generic ROOM sketch (Under development)
//
// ATTENTION:
// 1)
// 2) Before flashing, check that
// - Smoke sensor is deactivated if not installed (line 291?)
// - All customization variables are filled-out
//   ex:
//   - Temperature sensor ID
//   - For controlling the lights: Set "Nightlevel" global to a practical value (eg: 150 LUX) depending on what is used to measure it.
//
// --------------------------------------
// Functions:
//
// Roomsensebox connector (see below table)
// - D7 + A2 = Dust (smoke) sensor can be used instead of heated gas sensor
//
// OP connector (Actual use: see below table)
// for example:
// - A4 = Room CO2 ppm level
// - A5 = PIR2 movement sensor: Triggers a second group of lights
// - A6 = Room thermostat: Control heating in "Home" mode.
// - A7: Exterior lighting control with simple PWM signal (Pin A7) Two exterior LEDs 300 mA in series, controlled by dimmer module (A7), triggered by 1 exterior PIR sensor (A5)
// - A7: Laser barrier detects opening door(s): An LDR triggers the cabinet lights ON under a value.
//
// Day/Night signal: Catching "solar sensor" value broadcast by another controller...
//
// T-BUS (D3)
// - 1-wire address scanner: List all DS18B20 sensors!
// - Control heating in "HOME" mode: Follow the (manual) thermostat setting + Condens protection like in "OUT" mode.
// - Control heating in "OUT" mode (or for Condens protection in "Home" mode)
//   Follow Tout, the condens lower limit (Dewpoint +2°C), but limit the room temperature to 16°C (It is very seldom that a higher temp will be needed to protect against condense...)
//   => Use "Hysteresis" or longer getTemperaturesInterval (600s) to avoid too frequent switching...
// - ALERT is sent from IFTTT when the room temperature is close to the Dewpoint.
//
// Interior lighting (D4 - PIXEL-line)
// 1) Control a chain of PowerPixels: You can set the RGB values with the "colour picker" for that room.
// - PIR1 area: RoomsensePIR + RoomsenseLight
// - Spanlampen boven + Kleerkast: KleerkastPIR + RoomsenseLight
//
// 2) Let the LEDs around the roomfloor breathe all together...
//   Note 1: For the time being, no "breathing"!
//    Instead, I integrated RGB control with a "Picker" from a web form:
//    This was made by Particle community member @makerken: https://community.particle.io/t/simple-rgb-led-control-from-a-web-post-command/36520
//   Note 2: Put all PowerPixels first in the string (as LED 0) and then the series of BREATHING_LEDS.
//    Reason: If one of the BREATHING_LEDS fails, the first LED (PowerPiXel) will still keep it's position...
//
// 16-key touchpad: Used to select different modes for lighting etc...
//
// ---------------------------------------
// PhotoniX shield v.3.0	I/O connections: (* = used)
//
// *D0 - I2C-SDA           => OLED display
// *D1 - I2C-SCL           => OLED display
// *D2 - TOUCH-COM
// *D3 - RoomSense T-BUS
// *D4 - PIXEL-line
// *D5 - RoomSense PIR1: Entrance & Staircase lights
// *D6 - RoomSense TEMP/HUM
// *D7 - RoomSense GAS-DIG => DUST sensor (LED transmitter)
// *A0 - TOUCH-1
//  A1 - TOUCH-2 or DOTSTAR-line (Data)
// *A2 - RoomSense GAS-ANA => DUST sensor (LED receiver)
// *A3 - RoomSense LIGHT   => Put 22k series resistor with LDR (Not 5V tolerant!)
// *A4 - OP3               => CO2 PWM (Based on "CUBIC PWM CO2 Monitor.ino" =>  Duty Cycle analyzer for PWM CO2 sensors)
// *A5 - LCD-Reset / DOTSTAR-line (Clock) = OP4   => PIR2: Second light group
// *A6 - OP1 (Not 5V!)     => Thermostat
// *A7 - OP2               => LASER light sensor (LDR) = "LaserLDRpin"
//  TX/RX - Serial comms
//  ---------------------------------------
//
// To do:
// - Test 16-keypad & Dual I2C OLED display
// - Test DOTSTAR-line op A1 (Data) + A5 (Clock)
// - Test gas sensor op D7&A2 ipv dust sensor
//
//

// Specific ROOM settings:
  // 1. Choose ONE of below DS18B20 temperature sensors by uncommenting ONE line:
    byte addrs0[1][8] = {{0x28,0xFF,0x43,0x6D,0x33,0x17,0x04,0x3B}}; // INKOM room: New Round RoomSenseBoX (28,FF,43,6D,33,17,04,3B)
    //byte addrs0[1][8] = {{0x28,0xFF,0x3E,0x41,0x33,0x17,0x04,0xD8}}; // B&B room:  Old Square RoomSenseBoX (28,FF,3E,41,33,17,04,D8)


// GENERAL settings:
  STARTUP(WiFi.selectAntenna(ANT_AUTO)); // FAVORITE: continually switches at high speed between antennas
  SYSTEM_MODE(AUTOMATIC);

  // WiFi reception
  double wifiRSSI;
  double wifiPERCENT;

  // Memory monitoring
  double freemem;
  double memPERCENT;

  // String for publishing variables
  char str[255];

  // For storing Particle device NAME in event name groups:
  char device_name[32] = "";
  char stat_ROOM[64];
  char stat_LIGHT[64];
  char stat_HEAT[64];
  char stat_ALERT[64];

  // Homebridge reporting
  int HomebridgeInterval = 60 * 1000;
  int HomebridgeLastTime = millis() - HomebridgeInterval;

  // Sunlight level (Broadcasted by another Photon)
  double SUNLightlevel = 0; // Sunlight level from the (external) SOLAR sensor => Initially set to NIGHT- or DAYtime level (Value will be set from a "broadcast message" from a "daylight controller.
  int Nightlevel = 150; // Below this SUNLightlevel it is NIGHT! (broadcasted by another Photon running "I2C-Daylightsensor.ino")
  boolean itisnight = 0; // This will be set to 1 or 0, depending on "checkDAY_NIGHT()" function. (Initialized as DAY)
  String TIMEstatus = "Initialized as DAY!"; // Temporary string

// *A0-D2-Single-16touch-keypad
  #define kpCLOCKPERIODE 2                           // at least x µs LOW and the same HIGH

  const byte kpSDO[] = { A0 };                       // Data pins (SDO) of multiple KeyPads (Any I/O)
  const byte kpSCL   = D2;                           // Clock pin (SCL), common to multiple KeyPads (Any I/O. You can use D7 to see it work!)

  const int  kpCount = sizeof(kpSDO);                // retrieve count of keypads from dynamic array
  volatile uint16_t btnState[kpCount];
  char keys[sizeof(kpSDO)][17];

  Timer kpTimer(100, scan);                          // Software Timer to scan every 100ms = Run the function scan()

// * A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG
  int DUSTPin = A2;
  int LEDPin = D7;

  double DIGout = 0;
  double Vout = 0;
  double Dust = 0;// Double to be published!

  int getDustInterval = 8 * 1000;
  int getDustLastTime = millis() - getDustInterval;

// *A3 - RoomSense LIGHT
  int LIGHTpin = A3;
  double ROOMLightlevel;

  int getLightInterval = 2 * 1000;
  int getLightLastTime = millis() - getLightInterval;

// *A4 - OP3 => CO2 PWM
  // In this function pin A4 is specified in the loop()
  double pulseTime, CO2ppm;  // Use floating variables for calculations
  int CO2_R, CO2_G, CO2_B;
  int getCO2Interval = 20 * 1000; // Sample rate for CO2
  int getCO2LastTime = millis() - getCO2Interval;

// *A5 - PIR2: Second light group
  int PIR2pin = A5;
  int PIR2counter = 0; // Counts PIR2 triggers every second
  int PIR2counterLastTime;
  double PIR2movement = 0; // Total movement over last 5 minutes
  int PIR2totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter (5 min like Atomiot interval)
  int PIR2totalLastTime;
  String PIR2movstatus = "PIR2 empty"; // Initial setting
  boolean PIR2occup = 0; // Is this occupied?
  String PIR2lightstatus = "PIR2 light is OFF"; // Initial setting
  boolean PIR2light = 0; // Is this light ON?
  int PIR2LightONtime = 2 * 60 * 1000; // How long should the lights stay ON? (2 min)
  int PIR2LightONLastTime;

// *A6 - OP1 (Not 5V!) => Thermostat
  int TSTATpin = A6;
  String tstatTEMPstatus = "tstatTEMPstatus?"; // Temporary string
  int getTstatInterval = 10 * 1000; // Sample rate for Tstat
  int getTstatLastTime = millis() - getTstatInterval;  // Reset Tstat reading interval to sample immediately at start-up!

// *A7 - OP2 => LASER light sensor (LDR) = "LaserLDRpin" (for CABinet lights)
  int LaserLDRpin = A7;
  String CABmovstatus = "Laserbeam doors closed"; // Temporary string
  String CABlightstatus = "Laserbeam lights OFF"; // Temporary string
  String laser = "on"; // Initialize the laserbeam to "ON" (=AUTO position) => Can be turned off with function manual()
  double CabinetLaserLevel = 0; // Initial value of Laser LDR at the cabinet: Lights ON
  int CABlightOnTime = millis(); // Records ON time for the CAB lights
  int MaxCABlightOnTime= 10 * 60 * 1000;// Max ON time for the CAB lights: 10 min? (In case door is left open...)
  int getLaserInterval = 2 * 1000;
  int getLaserLastTime = millis() - getLaserInterval;

// *D3 - RoomSense T-BUS
  #include <OneWire.h>
  const int oneWirePin = D3;  // D3 = I2C-BUS (Check: 4.7K resistor to Vcc!)
  OneWire ds = OneWire(oneWirePin);

  // Initialize the names of the sensors as double variables: (=> can be published as "Particle.variables")
  double ROOMTemp1;
  double* temps[] = {&ROOMTemp1};

  // Initialize the globals for time stamp (Faulty sensor reporting via CRC checking):
  char crcErrorJSON[128];
  int crcErrorCount[sizeof(temps)/sizeof(temps[0])];
  uint32_t tmStamp[sizeof(temps)/sizeof(temps[0])];

  // Initialize the global variables for temperature calculations:
  int getTemperaturesInterval = 600 * 1000; // Sample rate for temperatures (s): To avoid "nervous" frequent heating ON/OFF switching, set this interval high enough!
  int getTemperaturesLastTime = millis() - getTemperaturesInterval;  // Reset heating setting interval to sample immediately at start-up!
  double celsius; // Holds the temperature from the array of sensors "per shot"...
  double Tout = 18; // Temperature setting when out of home
  String outTEMPstatus = "outTEMPstatus?"; // Temporary string

// *D4 - PIXEL-line
  #include <neopixel.h>
  #define PIXEL_COUNT 20
  #define PIXEL_PIN D4
  #define PIXEL_TYPE WS2812
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
  int rgb[3];

// *D5 - RoomSense PIR1: Entrance & Staircase
  int PIR1pin = D5;
  int PIR1counter = 0; // Counts PIR1 triggers every second
  int PIR1counterLastTime;
  double PIR1movement = 0; // Total movement over last 5 minutes
  int PIR1totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter (5 min like Atomiot interval)
  int PIR1totalLastTime;
  String PIR1movstatus = "PIR1 empty"; // Initial setting
  boolean PIR1occup = 0; // Is this occupied?
  String PIR1lightstatus = "PIR1 light is OFF"; // Initial setting
  boolean PIR1light = 0; // Is this light ON?
  int PIR1LightONtime = 5 * 60 * 1000; // How long should the lights stay ON? (5 min)
  int PIR1LightONLastTime;

// *D6 - RoomSense TEMP/HUM
  #include "math.h"             // to calculate mathematical functions! Needed???
  #include <PietteTech_DHT.h>   // "BEST Library" according to Particle community...
  #define DHTTYPE  DHT22        // Sensor type DHT11/21/22/AM2301/AM2302
  #define DHTPIN   D6           // Digital pin for communications
  PietteTech_DHT DHT(DHTPIN, DHTTYPE);
  int n;                        // counter

  double ROOMTemp2;
  double ROOMHumi;
  double ROOMTdf;       // Dewpoint temperature
  int SafeMargin = 2;   // Safety margin between Dewpoint and Set Minimum temperature. (Was 5: Too hot! Reduced to 3...)
  double ROOMTout;      // We can safely let the room temperature sink to this level when nobody is home for a long time.
  double Tdiff;         // The difference between ROOMTemp2 and ROOMTdf (difference with condens limit)

  int getTempHumInterval = 10 * 1000; // Minimum 2s!
  int getTempHumLastTime = millis() - getTempHumInterval;








void setup()
{
// GENERAL Settings:
  Time.zone(+1); // Set clock to Belgium time

// GENERAL Particle functions:
  Particle.function("Manual", manual);

// Initialize Particle subscribe function:
  // Catching private events
  Particle.subscribe("Status-", eventDecoder, MY_DEVICES); // Subscribe will listen for the event "Status-*" and, when it receives it, it will run the function eventDecoder()
  // Catching the device NAME
  Particle.subscribe("particle/device/name", DevNamehandler); // Listening to the particle cloud for device name...
  Particle.publish("particle/device/name"); // Ask the cloud (once) to send the device NAME!


// *A0-D2-Single-16touch-keypad
  pinMode(kpSCL, OUTPUT);                          // clock output
  pinSetFast(kpSCL);                               // set clock default HIGH (for active LOW setting)

  for (int i=0; i < kpCount; i++)                  // set all data pins as
    pinMode(kpSDO[i], INPUT);                      // input

  Particle.variable("keypad", (char*)keys);        // expose variable when cloud connected

  kpTimer.start();                                 // start sampling every 100ms


// *A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG
  pinMode(LEDPin,OUTPUT);
  pinMode(DUSTPin,INPUT_PULLUP); // Dust sensor. Input forced HIGH. <analogread> automatically switches pullup OFF (https://docs.particle.io/reference/firmware/photon/#pinmode-)
  Particle.variable("DUSTlevel", &Dust, DOUBLE);


// *A3 - RoomSense LIGHT
  pinMode(LIGHTpin,INPUT_PULLDOWN); // Light sensor. Input forced LOW. (If no sensor connected: 0)
  Particle.variable("ROOM_Light", &ROOMLightlevel, DOUBLE);


// *A4 - OP3 => CO2 PWM
  Particle.variable("CO2ppm", &CO2ppm, DOUBLE);


// *A5 - PIR2:
  pinMode(PIR2pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
  Particle.variable("PIR2movement", &PIR2movement, DOUBLE);


// *A6 - OP1 (Not 5V!) => Thermostat
  pinMode(TSTATpin, INPUT_PULLUP); // Thermostat. Input forced HIGH.


// *A7 - OP2 => LASER sensor (LDR) = "LaserLDRpin"
  pinMode(LaserLDRpin, INPUT); // LASER sensor (LDR) input

  // TEMPORARY for testing purpose: Check cabine operation!
  Particle.variable("LaserLevel", &CabinetLaserLevel, DOUBLE);

// *D3 - RoomSense T-BUS
  for (int i = 0; i < sizeof(temps)/sizeof(temps[0]); i++)
  {
    tmStamp[i] = Time.now();
  }
  //Particle.variable("CRC_Errors", crcErrorJSON, STRING); // Only needed for debugging: Shows accumulated number of errors...
  Particle.variable("ROOMTemp", &ROOMTemp1, DOUBLE);


// *D4 - PIXEL-line
  Particle.function("rgb", ledrgb);
  strip.begin(); strip.show(); // Initialize all pixels to 'off'

  // Initialize RGB color, until "mobile color picker" is used (Maarten's choice!);
    // ATTENTION: This must be in setup() !!!
  rgb[0] = 219; // Red      (0-255)
  rgb[1] = 159; // Green    (0-255)
  rgb[2] = 61;  // Blue     (0-255)


// *D5 - RoomSense PIR1: Entrance & Staircase
  pinMode(PIR1pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
  Particle.variable("PIR1movement", &PIR1movement, DOUBLE);


// *D6 - RoomSense TEMP/HUM
  Particle.variable("ROOM_Temp2", &ROOMTemp2, DOUBLE);
  Particle.variable("ROOM_Humid", &ROOMHumi, DOUBLE);
  Particle.variable("ROOM_Tout", &ROOMTout, DOUBLE);
}






void loop()
{
// General commands: Check memory
  freemem = System.freeMemory();// For debugging: Track if memory leak exists...
  memPERCENT = (freemem/51944)*100;

  if(memPERCENT < 50)
  {
   Particle.publish(stat_ALERT, "MEMORY LEAK in controller: Restarting!",60,PRIVATE);
   System.reset();
  }

  // Check if it is day or night:
  checkDAY_NIGHT();

  // REPORT to Homebridge:
   if ((millis()-HomebridgeLastTime)>HomebridgeInterval)
   {
    Particle.publish("co2value", "co2ppm=" + String(CO2ppm,1)); delay(500);// As the Homebridge Particle plugin does not yet allow gas sensors, you must call it a Humidity sensor
    Particle.publish("tvalue", "temperature=" + String(ROOMTemp1,1)); delay(500); // Roomtemperature
    Particle.publish("hvalue", "humidity=" + String(ROOMHumi,0)); delay(500); // Room humidity
    Particle.publish("lvalue", "light=" + String(ROOMLightlevel,0)); delay(500); // Room light

    HomebridgeLastTime = millis();
   }


// *A0-D2-Single-16touch-keypad
  static uint16_t oldState[kpCount];               // var to detect change
  bool needPrint = false;                          // flag whether any toggle button state needs to be printed

  for (int kp=0; kp < kpCount; kp++)               // iterate over all keypads
  {

    if(btnState[kp] != oldState[kp])               // if there was a change on the keypad
    {
      needPrint = true;
      oldState[kp] = btnState[kp];                 // remember new value for next time round
      for(int i=0; i<16; i++)                      // build a string for easy display
        keys[kp][i] = (btnState[kp] & (1 << i)) ? 'T' : '_';
        keys[kp][16] = '|';

    // Debug: Print the keypad status to serial monitor:
      if (kp == kpCount-1 && needPrint)
      {
        ((char*)keys)[sizeof(keys)-1] = '\0';
        Serial.println((char*)keys);
      }

      //  if (btnState[kp] == 0xFFFF)               // Touch all keys to connect to cloud (Only works if keypad is multi-touch configured)
      //      Particle.connect();

      // Pad 1: Create 16 boolean variables: One of the Pads touched?
      boolean Key1 = (btnState[0] >> 0) & 0x0001;
      boolean Key2 = (btnState[0] >> 1) & 0x0001;
      boolean Key3 = (btnState[0] >> 2) & 0x0001;
      boolean Key4 = (btnState[0] >> 3) & 0x0001;
      boolean Key5 = (btnState[0] >> 4) & 0x0001;
      boolean Key6 = (btnState[0] >> 5) & 0x0001;
      boolean Key7 = (btnState[0] >> 6) & 0x0001;
      boolean Key8 = (btnState[0] >> 7) & 0x0001;
      boolean Key9 = (btnState[0] >> 8) & 0x0001;
      boolean Key10 = (btnState[0] >> 9) & 0x0001;
      boolean Key11 = (btnState[0] >> 10) & 0x0001;
      boolean Key12 = (btnState[0] >> 11) & 0x0001;
      boolean Key13 = (btnState[0] >> 12) & 0x0001;
      boolean Key14 = (btnState[0] >> 13) & 0x0001;
      boolean Key15 = (btnState[0] >> 14) & 0x0001;
      boolean Key16 = (btnState[0] >> 15) & 0x0001;

      // 2) Run a corresponding function if a Key is touched
      if (Key1 == TRUE)
      {
        FunctionKey1();
      }

      if (Key2 == TRUE)
      {
        FunctionKey2();
      }

      if (Key3 == TRUE)
      {
        FunctionKey3();
      }

      if (Key4 == TRUE)
      {
        FunctionKey4();
      }

      if (Key5 == TRUE)
      {
        FunctionKey5();
      }

      if (Key6 == TRUE)
      {
        FunctionKey6();
      }

      if (Key7 == TRUE)
      {
        FunctionKey7();
      }

      if (Key8 == TRUE)
      {
        FunctionKey8();
      }

      if (Key9 == TRUE)
      {
        FunctionKey9();
      }

      if (Key10 == TRUE)
      {
        FunctionKey10();
      }

      if (Key11 == TRUE)
      {
        FunctionKey11();
      }

      if (Key12 == TRUE)
      {
        FunctionKey12();
      }

      if (Key13 == TRUE)
      {
        FunctionKey13();
      }

      if (Key14 == TRUE)
      {
        FunctionKey14();
      }

      if (Key15 == TRUE)
      {
        FunctionKey15();
      }

      if (Key16 == TRUE)
      {
        FunctionKey16();
      }
    } // END if there was a change on the keypad
  }


// * A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG => Used by a DUST sensor...
 if ((millis()-getDustLastTime) > getDustInterval)
 {
  ReadDust();
  // Put eventual extra actions/calculations with DUST here...
  if(Dust > 40 && Dust <= 80)
  {
   Particle.publish(stat_ALERT, "SMOKE or dust!",60,PRIVATE);
  }

  if(Dust > 80)
  {
   Particle.publish(stat_ALERT, "HEAVY SMOKE or dust!",60,PRIVATE);
  }
  getDustLastTime = millis();
 }


// *A3 - RoomSense LIGHT
 if ((millis()-getLightLastTime) > getLightInterval)
 {
  ReadLight();
  // Put eventual extra actions/calculations with LIGHT here...
  getLightLastTime = millis();
 }


// *A4 - OP3 => CO2 PWM
 if ((millis()-getCO2LastTime) > getCO2Interval)
 {

  checkCO2levelPWM(A4); // Read pin A4 (via OP connector) => CO2ppm ; Remark: In this function the pin is specified in the loop()

  // CO2 INDICATOR: The bathroom nightLEDs indicate the CO2 level: Red increases, Green decreases...
  CO2_R = constrain((CO2ppm / 5), 0, 255);
  CO2_G = constrain((255 - (CO2ppm / 5)), 0, 255);
  CO2_B = 0;

  /* Voor Lena de 4 "CO indicator LEDs" in badkamer tijdelijk uitschakelen! Later opnieuw inschakelen...
  for(int i=0;i<4;i++)
  {
   strip.setPixelColor(i, strip.Color(CO2_R, CO2_G, CO2_B));
   delay(100);
   strip.show();
  }
  */

  // CO2 ALERT:
  if (CO2ppm > 8) // CO2 level high!
  {
    Particle.publish(stat_ROOM, "Room CO2 ppm HIGH",60,PRIVATE); delay(500);
  }

  if (CO2ppm > 18) // CO2 level too high!
  {
    Particle.publish(stat_ROOM, "Room CO2 ppm too HIGH",60,PRIVATE); delay(500);
  }

 getCO2LastTime = millis(); // Reset timer
 }


// *A5 - PIR2: Cabinet room
 if (digitalRead(PIR2pin) == HIGH) // Motion detected
 {
  if ((millis()-PIR2counterLastTime) > 1000)
  {
   PIR2counter = PIR2counter + 1;
   PIR2counterLastTime = millis(); // Reset 1 minute period
  }
  PIR2LightONLastTime = millis(); // At each PIR trigger: reset ROOMlights turn OFF interval

  if (!PIR2occup)  // Not occupied
  {
   PIR2movstatus = "PIR2 in use"; Particle.publish(stat_ROOM, PIR2movstatus, 60,PRIVATE); delay(500);
   PIR2occup = 1;

   if (itisnight && !PIR2light)
   {
    PIR2Lightson();
   }
  }
 }

 else // No motion detected
 {
  if (PIR2occup) // No motion, Not yet published
  {
   PIR2movstatus = "PIR2 empty"; Particle.publish(stat_ROOM, PIR2movstatus, 60,PRIVATE); delay(500);
   PIR2occup = 0;
  }

  if (PIR2light && (millis()-PIR2LightONLastTime) > PIR2LightONtime) // No motion, lights are ON and light time is up
  {
   PIR2Lightsoff();
  }
 }


 if ((millis()-PIR2totalLastTime) > PIR2totaltime) // Are the 5 min totalling time over?
 {
  PIR2movement = PIR2counter; // PIR2movement is now published!
  PIR2counter = 0; // Reset counter!
  PIR2totalLastTime = millis(); // Reset 5 minute totalling period
 }




// *A6 - OP1 (Not 5V!) => Thermostat: Reports to HVAC controller if there is heat demand.
 if ((millis()-getTstatLastTime)>getTstatInterval) // This interval can be more frequent, as a thermostat usually has an "hysteresis" of 1°C
 {
  if (digitalRead(TSTATpin) == LOW) // Pin connected to GND = Heat demand!
  {
   tstatTEMPstatus = "Temperature below Tstat"; // For roomsensor area
  }
  else
  {
   tstatTEMPstatus = "Temperature above Tstat"; // For roomsensor area
  }
  Particle.publish(stat_HEAT, tstatTEMPstatus, 60,PRIVATE); delay(500);
  getTstatLastTime = millis(); // Reset the timer
 }


// OPGEPAST: May be temporarily switched off when laser beam is not correctly set...


// *A7 - OP2 => LASER sensor (LDR) = "LaserLDRpin"
 if ((millis()-getLaserLastTime) > getLaserInterval) // => It's time to check the LDR signal
 {
  readLaser();

  if (CabinetLaserLevel < 500 && CABmovstatus != "Laser cabinet doors open") // Door open => CabinetLaserLevel LOW and status changed => lights ON
  {
   CABmovstatus = "Laser cabinet doors open"; Particle.publish(stat_ROOM, CABmovstatus, 60,PRIVATE); delay(500);
   // Only turn CAB light ON if laser is "on" (enabled). If problems with laser beam, turn laser "off" with function manual("laseroff")...
   if (laser == "on") // Laser barrier is enabled (= Default)
   {
    CABlightsON(); // Turn cabinet lights ON!
   }
  }

  // If CABINET light ON time is expired, temporarily disable laser and turn CAB lights OFF. (Door left open!)
  if ((millis()-CABlightOnTime) >= MaxCABlightOnTime && CABmovstatus != "Laser cabinet doors closed") // Doors seem to be open and time is up!
  {
   CABlightsOFF();
   laser = "off"; // Disable the laser barrier
  }

  if (CabinetLaserLevel >= 500 && CABmovstatus != "Laser cabinet doors closed") // Doors closed => CabinetLaserLevel HIGH and status changed => lights OFF
  {
   CABlightsOFF(); // Turn Laser cabinet lights OFF!
   CABmovstatus = "Laser cabinet doors closed"; Particle.publish(stat_ROOM, CABmovstatus, 60,PRIVATE); delay(500);
   laser = "on"; // When the laser reaches the LDR, make sure the laserbarrier is enabled!
  }

  getLaserLastTime = millis(); // Reset timer
 }




// *D3 - T-BUS: Reports to HVAC controller if there is condens danger.
 if ((millis()-getTemperaturesLastTime)>getTemperaturesInterval) // To avoid "nervous" frequent switching, set this interval high enough!
 {
  getTemperatures(0); // Update all sensor variables of array 0 (DS18B20 type)
  // Actions/calculations with T-BUS output:
  if (ROOMTemp1 < Tout) // Report if room temperature is close to the condensation limit (Tout is a few degrees higher for safety!) Added a max limit for Tout of 16°C...
  {
   outTEMPstatus = "Temperature below Tout";
  }

  else
  {
   outTEMPstatus = "Temperature above Tout";
  }

  Particle.publish(stat_HEAT, outTEMPstatus, 60,PRIVATE); delay(500);
  getTemperaturesLastTime = millis(); // Reset the timer
 }


// *D4 - PIXEL-line: 10 pixels in 2 groups: 4 @ shower = 0-3 (CO2 indicator), 6 @ Bed = 4-9 (Adjustable with color picker)
// Opgepast: Hieronder 2 wijzigingen aangebracht voor Lena: Badkamerleds ook met colorpicker laten kiezen...
/*
  // 1. Turn the 6 LEDs (4-9) below the bed ON @ night. Use the "Picker" web form to vary color!
   if (itisnight == 1)
   {
    for(int i=4;i<10;i++)// Enkel die aan 't bed
    {
     strip.setPixelColor(i, strip.Color(rgb[0], rgb[1], rgb[2]));
     delay(100);
     strip.show();
    }
   }
   else // during daytime
   {
    for(int i=4;i<10;i++)// Enkel die aan 't bed
    {
     strip.setPixelColor(i, strip.Color(0, 0, 0));
     delay(100);
     strip.show();
    }
   }
*/


// *D5 - RoomSense PIR1: Entrance & Staircase
 if (digitalRead(PIR1pin) == HIGH) // Motion detected
 {
  if ((millis()-PIR1counterLastTime) > 1000)
  {
   PIR1counter = PIR1counter + 1;
   PIR1counterLastTime = millis(); // Reset 1 minute period
  }
  PIR1LightONLastTime = millis(); // At each PIR trigger: reset ROOMlights turn OFF interval

  if (!PIR1occup) // Not occupied
  {
   PIR1movstatus = "PIR1 in use"; Particle.publish(stat_ROOM, PIR1movstatus, 60,PRIVATE); delay(500);
   PIR1occup = 1;

   if (itisnight && !PIR1light)
   {
    PIR1Lightson();
   }
  }
 }

 else // No motion detected
 {
  if (PIR1occup) // No motion, Not yet published
  {
   PIR1movstatus = "PIR1 empty"; Particle.publish(stat_ROOM, PIR1movstatus, 60,PRIVATE); delay(500);
   PIR1occup = 0;
  }

  if (PIR1light && ((millis()-PIR1LightONLastTime) > PIR1LightONtime)) // No motion, lights are ON and light time is up
  {
   PIR1Lightsoff();
  }
 }


 if ((millis()-PIR1totalLastTime) > PIR1totaltime) // Are the 5 min totalling time over?
 {
  PIR1movement = PIR1counter; // PIR1movement is now published!
  PIR1counter = 0; // Reset counter!
  PIR1totalLastTime = millis(); // Reset 5 minute totalling period
 }



// *D6 - RoomSense TEMP/HUM
 if ((millis()-getTempHumLastTime)>getTempHumInterval)
 {
  ReadTempHum();
  // Put eventual actions/calculations with Temp & Hum here...
  Tout = ROOMTout;              // Replace the fixed value by the dynamic humidity limit.
  Tdiff = (ROOMTemp2 - ROOMTdf);// How far are we from condens limit?
  Tdiff = abs(Tdiff);           // Use the absolute value

  if (Tdiff < 1) // Warn for condensation risk: If the room temperature is close to the dewpoint
  {
    Particle.publish(stat_ALERT, "CONDENS DANGER!", 60,PRIVATE); delay(500); // This is picked up by IFTTT => Notification sent to my iPhone
    sprintf(str, "Tdiff: %2.1f",Tdiff); Particle.publish(stat_HEAT, str,60,PRIVATE); delay(500);
  }

  getTempHumLastTime = millis();
 }

} // end loop()









// FUNCTIONS:

// General: Function for lighting application: Is it night?
void checkDAY_NIGHT() //Set the conditions considered as "night": Read a solar sensor (outside light sensor) 0-100% by another controller ans broadcast value. Receive the value from the solar sensor in the "message decoder function"...
{

  if(SUNLightlevel > Nightlevel) // SUNLightlevel = received from solar sensor (On another controller), Nightlevel is SET to a value on top of this sketch...
  {
    itisnight = 0; // it is day!
    if (TIMEstatus != "It is DAYTIME")
    {
     TIMEstatus = "It is DAYTIME"; Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE); delay(500);
    }
  }

  else // If SUNLightlevel <= Nightlevel
  {
    itisnight = 1; // It is night!
    if (TIMEstatus != "It is NIGHT")
    {
     TIMEstatus = "It is NIGHT"; Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE); delay(500);
    }
  }

  /* Alternatively: Use the clock time to determine if it is day or night.
  if (Time.hour() >= 18 || Time.hour() <= 8) // = Night if hour is 18 19 20 21 22 23 24 0 1 2 3 4 5 6 7 8 (Belgium time)
  {
    //Nighttime
  }
  else
  {
    // Daytime
  }
  */
}





// * A0-D2-Single-16touch-keypad
void scan()
{
  for (int kp=0; kp < kpCount; kp++)
    btnState[kp] = 0;                              // reset current state

  for(int i=0; i<16; i++)
  {
    pinResetFast(kpSCL);                           // first set clock LOW
    delayMicroseconds(kpCLOCKPERIODE);             // wait for output data
    pinSetFast(kpSCL);                             // latch data with rising edge
    delayMicroseconds(kpCLOCKPERIODE);             // wait again
    for (int kp=0; kp < kpCount; kp++)
      btnState[kp] |= pinReadFast(kpSDO[kp]) << i; // set bit
  }
  for (int kp=0; kp < kpCount; kp++)
    btnState[kp] ^= 0xFFFF;                        // invert due to default setting (active LOW)
}

 // Functions per key: Examples...
void FunctionKey1()
{
  PIR1Lightson();
  Particle.publish("action","Key-1",60,PRIVATE); delay(500);
}

void FunctionKey2()
{
  PIR2Lightson();
  Particle.publish("action","Key-2",60,PRIVATE); delay(500);
}

void FunctionKey3()
{
  PIR1Lightson();
  PIR2Lightson();
  Particle.publish("action","Key-3",60,PRIVATE); delay(500);
}

void FunctionKey4()
{
  PIR1Lightsoff();
  Particle.publish("action","Key-4",60,PRIVATE); delay(500);
}

void FunctionKey5()
{
  PIR2Lightsoff();
  Particle.publish("action","Key-5",60,PRIVATE); delay(500);
}

void FunctionKey6()
{
  PIR1Lightsoff();
  PIR2Lightsoff();
  Particle.publish("action","Key-6",60,PRIVATE); delay(500);
}

void FunctionKey7()
{
  CABlightsON();
  Particle.publish("action","Key-7",60,PRIVATE); delay(500);
}

void FunctionKey8()
{
  CABlightsOFF();
  Particle.publish("action","Key-8",60,PRIVATE); delay(500);
}

void FunctionKey9()
{
  Particle.publish("action","Key-9",60,PRIVATE); delay(500);
}

void FunctionKey10()
{
  Particle.publish("action","Key-10",60,PRIVATE); delay(500);
}

void FunctionKey11()
{
  Particle.publish("action","Key-11",60,PRIVATE); delay(500);
}

void FunctionKey12()
{
  Particle.publish("action","Key-12",60,PRIVATE); delay(500);
}

void FunctionKey13()
{
  Particle.publish("action","Key-13",60,PRIVATE); delay(500);
}

void FunctionKey14()
{
  Particle.publish("action","Key-14",60,PRIVATE); delay(500);
}

void FunctionKey15()
{
  Particle.publish("action","Key-15",60,PRIVATE); delay(500);
}

void FunctionKey16()
{
  Particle.publish("action","Key-16",60,PRIVATE); delay(500);
}



// * A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG
void ReadDust()
{
 // The Sharp document describes the sampling scenario:
 digitalWrite(LEDPin,LOW);             //LED on
 delayMicroseconds(280);               // After 280 micros, the LED pulse is at it's peak
 DIGout = analogRead(DUSTPin);         // Read output once...
 delayMicroseconds(40);                // To reach 320 micros ON time, we need 40 micros more...
 digitalWrite(LEDPin,HIGH);            // Then, LED off!
 Dust = DIGout/20;                     // Scale down

 // Scale the output to obtain a 'percent like' output:
 // TEMPORARY: next line generates compile error... TEST this function later when needed!
 //Dust = map(DIGout, 0, 4095, 0, 250);   // Remap the readings (Experiment with value to saturate around 135)
 Dust = Dust - 70;                      // Subtract the 'normal value' around 70 in the room, to get a scale for extra dust from 0-100%... Test with incense stick!
 Dust = constrain(Dust, 0,100);         // Allow only values between 0 and 100
}

 /* Alternatively, smooth out the sampling:
 DIGout = 0;
 for(int x = 0 ; x < 20 ; x++) // Read average output of 20 samples!
 {
  // The Sharp document describes the sampling scenario:
  digitalWrite(LEDPin,LOW);             //LED on
  delayMicroseconds(280);               // After 280 micros, the LED pulse is at it's peak
  DIGout = DIGout + analogRead(DUSTPin);// Read output
  delayMicroseconds(40);                // To reach 320 micros ON time, we need 40 micros more...
  digitalWrite(LEDPin,HIGH);            // Then, LED off!
  delay(9);                             // Minimum period = 10 ms: If no other delay in the loop, 9 ms is the minimum to get good samples
 }
 DIGout = DIGout/20;                    // Calculate the average
 */





// *A3 - RoomSense LIGHT
void ReadLight()
{
  ROOMLightlevel = 0;

  for(int x = 0 ; x < 20 ; x++)
  {
   ROOMLightlevel = ROOMLightlevel + analogRead(LIGHTpin);//
  }

  ROOMLightlevel = ROOMLightlevel/20; // Average
  ROOMLightlevel = ROOMLightlevel/150; // = scaled for graphing together with other variables (Max value = 20)
}




// *A4 - OP3 => CO2 PWM
void checkCO2levelPWM(pin_t pulsePin)
{
  CO2ppm=0;
  int startTime;

  pinMode(pulsePin, INPUT_PULLUP);// Make the pin HIGH without signal

  startTime=micros(); //Start time to measure wait time

  while(digitalRead(pulsePin) == HIGH)  //Detect NO CONNECTION; Wait for LOW signal:
  {
    if ((micros()-startTime) > 1000000) // If longer than 0.7s, quit function! (Experimentally: As low as possible, without losing pulses)
    {
     return;// Exit the function!
    }
  }

  pulseTime=pulseIn(pulsePin, HIGH);
  CO2ppm=2*(pulseTime/1000)-2;
  CO2ppm = CO2ppm/100; // = scaled for Atomiot (Max value = 20)
}





// *A7 - OP2 => LASER sensor (LDR) = "LaserLDRpin"
void readLaser() // Check if laser hits LDR or not => CabinetLaserLevel => Control Cabinet lights
{
  CabinetLaserLevel = 0;
  for(int x = 0 ; x < 20 ; x++)
  {
   CabinetLaserLevel = CabinetLaserLevel + analogRead(LaserLDRpin);//
  }
  CabinetLaserLevel = CabinetLaserLevel/20; // Average
}




// *D3 - RoomSense T-BUS
// Read the DS18B20 precision room temperature sensor(s)...
void getTemperatures(int select)
{
    ds.reset();
    ds.skip();
    ds.write(0x44, 0);
    delay(1000);
    ds.reset();

    for (int i=0; i< sizeof(temps)/sizeof(temps[0]); i++)
    {
        ds.select(addrs0[i]);
        ds.write(0xBE,0);
        byte scratchpadData[9];

        for (int i = 0; i < 9; i++) // we only need 9 bytes
        {
          scratchpadData[i] = ds.read();
        }

        byte currentCRC = (OneWire::crc8(scratchpadData, 8));

        ds.reset();

        if (currentCRC != scratchpadData[8])
        {
            String message;
            if (Time.now() - tmStamp[i] > 3600UL)  // one hour in this example
            {
                message = "Sensor Timeout on ROOMsensor: ";
            }
            else
            {
               message = "Bad reading on ROOMsensor: ";
            }
            Particle.publish(stat_HEAT, message + String(i), 60, PRIVATE);
            crcErrorCount[i]++;
            delay(1000);
            continue;
        }

        tmStamp[i] = Time.now();

        int16_t raw = (scratchpadData[1] << 8) | scratchpadData[0];
        celsius = (double)raw * 0.0625;

        // construct the CRC error array
        strcpy(crcErrorJSON, "{\"errorCount\":[");
        for (int i = 0; i < sizeof(temps)/sizeof(temps[0]); i++)
        {
            char buffer[10];
            itoa(crcErrorCount[i], buffer, 10);
            strcat(crcErrorJSON, buffer);
            if(i < sizeof(temps)/sizeof(temps[0]) - 1)
              strcat(crcErrorJSON, ",");
        }
        strcat(crcErrorJSON, "]}");
        *temps[i] = celsius;
    }
}




// *D3 - T-BUS: 1-wire bus Scanner function  => Addresses of 1-wire devices are published to Particle cloud. For example T-BUS (pin D3) DS18B20 sensors
void discoverOneWireDevices(void)
{
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  int sensorCount = 0;

  Particle.publish("OneWire", "Looking for 1-wire addresses:", 60, PRIVATE); delay(500);

  while(ds.search(addr) and sensorCount < 20)
  {
    sensorCount++;
    char newAddress[48] = ""; // Make space for the 48 characters of our sensor addresses.
    snprintf(newAddress, sizeof(newAddress), "0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]); Particle.publish("OneWire", newAddress, 60, PRIVATE); delay(500);

    if ( OneWire::crc8( addr, 7) != addr[7])
    {
      Particle.publish(stat_ALERT, "CRC is not valid!", 60, PRIVATE); delay(500);
      return;
    }
  }

  ds.reset_search();
  Particle.publish("OneWire", "No more addresses!", 60, PRIVATE); delay(500);
  return;
}





// *D4 - PIXEL-line - PARTICLE FUNCTION

 /* Runs when a color is picked in the corresponding "INKOM_RGB.html" webform (with this Photon's ID nr!)
    This was made by Particle community member @makerken: https://community.particle.io/t/simple-rgb-led-control-from-a-web-post-command/36520
 */

int ledrgb( String value )
{
    String redgreenblue[3];
    String delimiter = "_";
    int j = 0;

    for (int i = 0; i < value.length(); i++)
    {
        if (value[i] == *delimiter)
        {
          j++;
        }
        else
        {
          redgreenblue[j] += value[i];
        }
    }

    // Store the picker RGB values in the array:
    rgb[0] = redgreenblue[0].toInt();
    rgb[1] = redgreenblue[1].toInt();
    rgb[2] = redgreenblue[2].toInt();

    // Publish the selected colour:
    sprintf(str, "Colour:%d-%d-%d",rgb[0],rgb[1],rgb[2]); Particle.publish(stat_LIGHT, str,60,PRIVATE); delay(500);

    // Turn on all related RGB lights to confirm the set color:
    PIR1Lightson();
    PIR2Lightson();

    return 1; // Feedback when successful...
}



// *D4 - PIXEL-Line: (Attention: First in string = 0!)
// ATTENTION: While studying the "flicker" issues, choose "dimming" or "simple switching" settings below!

// INKOM POWERPIXEL CONTROL:
void PIR1Lightson() // Turn lights ON.
{
// Powerpixel 0 = Spanlampen inkom: Select an option...
  //strip.setPixelColor(0, strip.Color(255,255,255)); strip.show(); // Simple switching option: R= Spanlampen beneden, B= Spots onder palier (SIMPLE SWITCH ON = For testing)
  strip.setPixelColor(0, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb()
  //DimUpGroups(0, 0, 5); // Dimming option: Check "flicker" issues! PowerPiXel 0 = SpanBeneden, Min = 0, Steps = 10 (slow)

// Powerpixel 2 = RGB strips: Select an option...
  //strip.setPixelColor(2, strip.Color(255,255,255)); strip.show(); // Simple switching option (For testing)
  //DimUpGroupsRG(2, 0, 5); // PowerPiXel 2 = RGB, Min = 0, Steps = 10 (slow)
  strip.setPixelColor(2, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb() above!

  PIR1lightstatus = "PIR1 light is ON"; Particle.publish(stat_LIGHT, PIR1lightstatus, 60,PRIVATE); delay(500);
  PIR1light = 1;
}

void PIR1Lightsoff() // Turn lights OFF per colour: R= Spanlampen beneden, G= Spots onder palier
{
// Simple switching option:
  strip.setPixelColor(0, strip.Color(0,0,0)); // SIMPLE SWITCH OFF = For testing
  strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // RGB lights (SIMPLE SWITCH OFF = For testing)

// Dimming option: Check "flicker" issues!
  //DimDownGroups(2, 0, 5); // PowerPiXel 2 = RGB, Min = 0, Steps = 10 (slow)
  //DimDownGroups(0, 0, 5); // PowerPiXel 0 = SpanBeneden, Min = 0, Steps = 10 (slow)

  PIR1lightstatus = "PIR1 light is OFF"; Particle.publish(stat_LIGHT, PIR1lightstatus, 60,PRIVATE); delay(500);
  PIR1light = 0;
}

// PIR2 POWERPIXEL CONTROL:
void PIR2Lightson() // Turn second group of lights (Powerpixel 1) ON
{
// Simple switching option:
  //strip.setPixelColor(1, strip.Color(255,255,255)); strip.show(); // Simple switching: R= Second roomlights (SIMPLE SWITCH ON = For testing)
  strip.setPixelColor(1, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Adopt the RGB values set remotely

// Dimming option:
  //DimUp(1, 0, 255, 5); // Spanlampen boven = PowerPiXel 1 (= second), Pixel #, Start - End, Nr of steps
  PIR2lightstatus = "PIR2 light is ON"; Particle.publish(stat_LIGHT, PIR2lightstatus, 60,PRIVATE); delay(500);
  PIR2light = 1;
}

void PIR2Lightsoff() // Turn second group of lights (Powerpixel 1) OFF
{
// Simple switching option:
  strip.setPixelColor(1, strip.Color(0,0,0)); strip.show(); // R= Second group of lights (SIMPLE SWITCH ON = For testing)

// Dimming option:
  //DimDown(1, 255, 0, 5); // Spanlampen boven = PowerPiXel 1 (= second), Pixel #, Start - End, Nr of steps

  PIR2lightstatus = "PIR2 light is OFF"; Particle.publish(stat_LIGHT, PIR2lightstatus, 60,PRIVATE); delay(500);
  PIR2light = 0;
}


void PIR2demo() // One cycle dimming ON/OFF...
{
  PIR2Lightson();
  delay(5000);
  PIR2Lightsoff();
}


// CABinet lights:
void CABlightsON() // Turn cabinet lights (Powerpixel 3) ON
{
  CABlightOnTime = millis(); // Restart the cabinet light ON timer. Will turn OFF lights after expiring...
  strip.setPixelColor(3, strip.Color(255,255,255)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power ON
  CABlightstatus = "Cabinet lights ON"; Particle.publish(stat_LIGHT, CABlightstatus, 60,PRIVATE); delay(500);
}

void CABlightsOFF() // Turn cabinet lights (Powerpixel 3) OFF
{
  strip.setPixelColor(3, strip.Color(0,0,0)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power OFF
  CABlightstatus = "Cabinet lights OFF"; Particle.publish(stat_LIGHT, CABlightstatus, 60,PRIVATE); delay(500);
}


// BASIC POWERPIXEL CONTROL FUNCTIONS which can be used for dimming:
// 1. Increases brightness of all channels of a PowerPixel in steps. Parameters: Pixel #, Start - End, Nr of steps (1 = fast, more = slower) ex: DimUp(0, 0, 255, 5);
void DimUp(uint8_t Nr, uint8_t min, uint8_t max, uint8_t wait)
{
  for(int i=min;i<max;i++)
  {
    strip.setPixelColor(Nr, strip.Color(i,i,i));
    strip.show();
    delay(wait);
  }
}

// 2. Decreases brightness of all channels of a PowerPixel in steps: Parameters: Pixel #, Start - End, Nr of steps (1 = fast, more = slower) ex: DimDown(0, 255, 0, 5);
void DimDown(uint8_t Nr, uint8_t max, uint8_t min, uint8_t wait)
{
  for(int i=max;i>min;i--)
  {
    strip.setPixelColor(Nr, strip.Color(i,i,i));
    strip.show();
    delay(wait);
  }
}

// 3. Fades each channel of a PowerPixel OFF in turn: Parameters: Pixel #, Minimum, Nr of steps (1 = fast, more = slower) ex: DimDownGroups(0, 0, 5);
void DimDownGroups(uint8_t Nr, uint8_t min, uint8_t wait)
{
  for(int i=255;i>min;i--)
  {
    strip.setPixelColor(Nr, strip.Color(i,255,255));
    strip.show();
    delay(wait);
  }
  for(int i=255;i>min;i--)
  {
    strip.setPixelColor(Nr, strip.Color(0,i,255));
    strip.show();
    delay(wait);
  }
  for(int i=255;i>min;i--)
  {
    strip.setPixelColor(Nr, strip.Color(0,0,i));
    strip.show();
    delay(wait);
  }
}

// 4a. Fades each channel of a PowerPixel ON in turn to WHITE: Parameters: Pixel #, Minimum, Nr of steps (1 = fast, more = slower) ex: DimUpGroups(0, 0, 5);
void DimUpGroups(uint8_t Nr, uint8_t min, uint8_t wait)
{
  for(int i=min;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(i,0,0));
    strip.show();
    delay(wait);
  }

  for(int i=min;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(255,i,0));
    strip.show();
    delay(wait);
  }
  for(int i=min;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(255,255,i));
    strip.show();
    delay(wait);
    }
}

// 4b. Fades each channel of a PowerPixel ON in turn to warm YELLOW: Parameters: Pixel #, Minimum, Nr of steps (1 = fast, more = slower) ex: DimUpGroups(0, 0, 5);
void DimUpGroupsRG(uint8_t Nr, uint8_t min, uint8_t wait)
{
  for(int i=min;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(i,0,0));
    strip.show();
    delay(wait);
  }

  for(int i=min;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(255,i,0));
    strip.show();
    delay(wait);
  }
}




// *D6 - RoomSense TEMP/HUM
void ReadTempHum()
{
 int result = DHT.acquireAndWait(1000); // wait up to 1 sec (default indefinitely)
 ROOMTemp2 = DHT.getCelsius();
 ROOMHumi = DHT.getHumidity();
 ROOMTdf = DHT.getDewPoint();
 ROOMTout = ROOMTdf + SafeMargin; // Keep a safe distance from the dewpoint...
 ROOMTout = constrain(ROOMTout, 4, 16); // Constrain ROOMTout between 4 and 16°C: More economical. Very seldom a higher T° will be needed to protect against condense...
 n++;
}



// COMMON Particle Function(s):

// Receive and decode the Particle status messages (with a name starting with "Status-"):
void eventDecoder(const char *event, const char *data) // This function is called when an event with name starting with "Status-" is published
{
  char* Subject = strtok(strdup(data), ":"); // Take first string until delimiter = ":" (If there is no : then the full string is copied!)

  // Decode variables: If "Subject" starts with a 3-character string followed by ":" => Create variable names, depending on the "Subject" name...
  // 1) SUNLightlevel: To be used to enable exterior lights.
  if (strncmp(Subject, "SUN", strlen("SUN")) == 0) // String = SUN
  {
    SUNLightlevel = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable
  }
  free(Subject); // To avoid memory leak, this memory must be freed every time...
}



// Receive the device NAME from Particle cloud
void DevNamehandler(const char *topic, const char *name)
{
  strncpy(device_name, name, sizeof(device_name)-1); // Store the device name in a string
  snprintf(stat_ROOM, sizeof(str), "Status-ROOM:%s", (const char*)device_name); // Create the Status-ROOM event name with the device name at the end
  snprintf(stat_LIGHT, sizeof(str), "Status-LIGHT:%s", (const char*)device_name); // Create the Status-LIGHT event name with the device name at the end
  snprintf(stat_HEAT, sizeof(str), "Status-HEAT:%s", (const char*)device_name); // Create the Status-HEAT event name with the device name at the end
  snprintf(stat_ALERT, sizeof(str), "Status-ALERT:%s", (const char*)device_name); // Create the Status-ALERT event name with the device name at the end
}



int manual(String command) // = Particle.function to remote control manually. Can also be called from the loop(): ex = manual("Lighton");
{
  if(command == "report")
  {
    // GENERAL
    // Date/time stamp
    Particle.publish(stat_ROOM, Time.timeStr(), 60,PRIVATE); delay(500); // eg: Wed May 21 01:08:47 2014
    sprintf(str, "SUNLIGHT:%2.0f",SUNLightlevel); Particle.publish(stat_LIGHT, str,60,PRIVATE); delay(500);
    Particle.publish(stat_ROOM, TIMEstatus, 60,PRIVATE); delay(500);

    // WiFi reception
    wifiRSSI = WiFi.RSSI();
    wifiPERCENT = wifiRSSI+120;
    sprintf(str, "RSSI+Pct:%2.0f+%2.0f",wifiRSSI,wifiPERCENT); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);

    // Memory monitoring
    memPERCENT = (freemem/51312)*100;
    sprintf(str, "Mem+Pct:%2.0f+%2.0f",freemem,memPERCENT); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);

    // ROOMSENSE box
    sprintf(str, "Room Light: %2.0f",ROOMLightlevel); Particle.publish(stat_LIGHT, str,60,PRIVATE); delay(500);
    sprintf(str, "Room Temp1: %2.1f",ROOMTemp1); Particle.publish(stat_HEAT, str,60,PRIVATE); delay(500);
    sprintf(str, "Room Temp2+Humid+Dew+Tout:%2.1f+%2.0f+%2.1f+%2.1f",ROOMTemp2,ROOMHumi,ROOMTdf,ROOMTout); Particle.publish(stat_HEAT, str,60,PRIVATE); delay(500);
    Particle.publish(stat_HEAT, outTEMPstatus, 60,PRIVATE); delay(500);
    Particle.publish(stat_HEAT, tstatTEMPstatus, 60,PRIVATE); delay(500);
    sprintf(str, "Room CO2 x100: %2.0f",CO2ppm); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    sprintf(str, "Room DUST Pct: %2.0f",Dust); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);

    // All RGB related lights: Selected RGB colour
    sprintf(str, "Colour:%d-%d-%d",rgb[0],rgb[1],rgb[2]); Particle.publish(stat_LIGHT, str,60,PRIVATE); delay(500);

    // PIR1 Lights
    sprintf(str, "PIR1movement: %2.0f",PIR1movement); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    Particle.publish(stat_ROOM, PIR1movstatus, 60,PRIVATE); delay(500);
    Particle.publish(stat_LIGHT, PIR1lightstatus, 60,PRIVATE); delay(500);

    // PIR2 Lights
    sprintf(str, "PIR2movement: %2.0f",PIR2movement); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    Particle.publish(stat_ROOM, PIR2movstatus, 60,PRIVATE); delay(500);
    Particle.publish(stat_LIGHT, PIR2lightstatus, 60,PRIVATE); delay(500);

    // Cabinet Laser barrier & Lights
    sprintf(str, "Laser LDR: %2.0f",CabinetLaserLevel); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    Particle.publish(stat_ROOM, CABmovstatus, 60,PRIVATE); delay(500);
    Particle.publish(stat_LIGHT, CABlightstatus, 60,PRIVATE); delay(500);

    // 1-wire sensors
    discoverOneWireDevices();

    return 1000;
  }

  if((command == "pir1on") || (command == "Lights=1}")) // Second condition is for Homebridge
  {
    PIR1Lightson();
    PIR1LightONLastTime = millis(); // Reset ROOMlights turn OFF interval
    return 100;
  }

  if((command == "pir1off") || (command == "Lights=0}")) // Second condition is for Homebridge
  {
    PIR1Lightsoff();
    return 1;
  }

  if((command == "pir2on") || (command == "EXT1=1}")) // Second condition is for Homebridge
  {
    PIR2Lightson();
    PIR2LightONLastTime = millis(); // Reset ROOMlights turn OFF interval
    return 200;
  }

  if((command == "pir2off") || (command == "EXT1=0}")) // Second condition is for Homebridge
  {
    PIR2Lightsoff();
    return 2;
  }

  if(command == "laseron") // Enable the cabinet laser barrier and let CAB lights automatically react to it
  {
    laser = "on";
    return 1;
  }

  if(command == "laseroff") // Disable the cabinet laser barrier and turn CAB lights OFF
  {
    laser = "off";
    CABlightsOFF();
    return 0;
  }

  if(command == "cabineton")
  {
    CABlightsON();
    return 300;
  }

  if(command == "cabinetoff")
  {
    CABlightsOFF();
    return 3;
  }

  if(command == "night") // Manual mode change if no sunlight info is received...
  {
    SUNLightlevel = 0; // Sunlight level is set to NIGHTtime level (Value will normally be set by a "broadcast message" from a "daylight controller.
    return 0;
  }

  if(command == "day") // Manual mode change if no sunlight info is received...
  {
    SUNLightlevel = 1000; // Sunlight level is set to DAYtime level (Value will normally be set by a "broadcast message" from a "daylight controller.
    return 1000;
  }

  if(command == "reset") // You can remotely RESET the photon with this command... (It won't reset the IO-eXbox!)
  {
    System.reset();
    return -10000;
  }

  Particle.publish(stat_ROOM, command,60,PRIVATE); delay(500); // If it does not match one of the above, publish the received string to see what it's "payload" was...
  return -1;// If none above
}
