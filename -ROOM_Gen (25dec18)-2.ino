// ROOM_Gen.ino = Generic ROOM sketch   -   Version 25dec18
//
// Purpose: This sketch puts life in a PhotoniX shield with it's accessories connected via the default ports (More below...)
//
// ---------------------------------------
// PhotoniX shield v.3.0	I/O connections: (* = used)
//
// *D0 & D1 (I2C-SDA & SCL) = OLED display, I/O expander, ...
// *D2 - TOUCH-COM = 16-key touchpad (CLK)
// *D3 - RoomSense T-BUS
// *D4 - OP6 = PIXEL-line
// *D5 - RoomSense PIR1: Entrance & Staircase lights
// *D6 - RoomSense TEMP/HUM
// *D7 - RoomSense GAS-DIG => DUST sensor (LED transmitter)
// *A0 - TOUCH-1 = 16-key touchpad (DATA1)
//  A1 - OP1 = DOTSTAR-line (Data) or TOUCH-2 = 16-key touchpad (DATA1)
// *A2 - RoomSense GAS-ANA => DUST sensor (LED receiver)
// *A3 - RoomSense LIGHT   => Put 22k series resistor with LDR (Not 5V tolerant!)
// *A4 - OP4 = PWM input for CO2
// *A5 - OP5 = PIR2 (Second light group), LCD-Reset or DOTSTAR-line (Clock)
// *A6 - OP2 (Not 5V!) = Thermostat
// *A7 - OP3 = PIR2 dimmer pin or LASER light sensor (LDR) input.
//  TX/RX - Serial comms
//  ---------------------------------------
//
// ATTENTION: Before flashing, check that:
// 1) All customization variables for OPTIONAL devices are filled-out below.
//   ex:
//    - Roomsense connector: Temperature sensor ID? Gas or Smoke sensor installed?
//    - OP connector: Which option installed?
//    - Commands for 16-key touchpad
//    - Commands for the Pixel line (for lighting control etc...)
// 2) Set other parameters like "Nightlevel" for this room. (eg: below 150 LUX it is NIGHT)
//
// --------------------------------------
// Functions:
//
// Roomsensebox connector (see below table)
// - The standard environmental data are measured: Room temperature (2), humidity and light value.
// - Optional data: D7 + A2 = Gas or Dust (smoke) sensor.
//
// OP connector (Actual use: see below table)
// for example:
// - A4 = Room CO2 ppm level (Based on "CUBIC PWM CO2 Monitor.ino" =>  Duty Cycle analyzer for PWM CO2 sensors)
// - A5 = PIR2 movement sensor: Triggers a second group of lights
// - A6 = Room thermostat: Control heating in "Home" mode.
// - A7 Option1: Lighting control with PWM signal. In BandB: Two 300 mA LEDs in series, controlled by a dimmer module, triggered by PIR2 sensor.
// - A7 Option2: Laser barrier detects opening door(s): In BandB: A light sensor triggers the door lights ON under a value.
//
// Day/Night signal: Catching "SUNlight" value broadcast by another controller...
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
// 1) PIR1 & PIR2 sensors: Control a chain of PowerPixels: You can set the RGB values with the "colour picker" for that room.
// 2) Let the LEDs around the roomfloor breathe all together...
//   Note 1: For the time being, no "breathing"!
//    Instead, I integrated RGB control with a "Picker" from a web form:
//    This was made by Particle community member @makerken: https://community.particle.io/t/simple-rgb-led-control-from-a-web-post-command/36520
//   Note 2: Put all PowerPixels first in the string (as LED 0) and then the series of BREATHING_LEDS.
//    Reason: If one of the BREATHING_LEDS fails, the first LED (PowerPiXel) will still keep it's position...
//
// 16-key touchpad:
// - Used to select different modes for lighting etc...
//
// OLED display:
// - Combined with 16-key touchpad.
// - It displays Particle cloud "Status" messages and feedback / instructions of the 16-key touchpad.
//   It alternates between this display and a display showing the main Room data.
//
//
// To do:
// - Integrate second OLED display: Show KEY instructions!
// - Test DOTSTAR-line op A1 (Data) + A5 (Clock)
// - Test gas sensor op D7&A2 ipv dust sensor

// START Specific ROOM settings for accessories: "Room-INKOM"////////////////////////////////////////////////////////////

// *D3: ROOMSENSE connector: Select DS18B20 temperature sensors in use by uncommenting line(s):
//byte addrs0[1][8] = {{0x28,0xFF,0x43,0x6D,0x33,0x17,0x04,0x3B}};  // INKOM room: New Round RoomSenseBoX
//byte addrs0[1][8] = {{0x28,0xFF,0x3E,0x41,0x33,0x17,0x04,0xD8}};  // BandB room:  Old Square RoomSenseBoX
byte addrs0[1][8] = {{0x28,0xFF,0x1A,0x3A,0x33,0x17,0x04,0x03}};  // Test roomsense module:  New Round RoomSenseBoX
// *A0 & D2: INTERFACE connector: NONE, TOUCH1, ...
char INT_A0[10] = "TOUCH1"; //
// *A1: OPTIONAL connector: NONE, DOTSTAR_D, TOUCH2, ...
char OP1_A1[10] = "NONE"; //
// *A2 & D7: ROOMSENSE connector: NONE, GAS, DUST, ...
char GAS_A2D7[10] = "DUST"; //
// *A4: OPTIONAL connector: NONE, CO2_PWM, ...
char OP2_A4[10] = "NONE"; //
// *A5: OPTIONAL connector: NONE, PIR2 (w PIXELs), DOTSTAR_C, RESET (LCD), ...
char OP3_A5[10] = "PIR2"; //
// *A6: OPTIONAL connector: NONE, TSTAT...
char OP4_A6[10] = "TSTAT"; //
// *A7: OPTIONAL connector: NONE, (PIR2) DIMMER, LASERRCV, ...
char OP5_A7[10] = "LASERRCV"; //
// *D0/D1: I2C connector: NONE, OLED, 16P_EXP, ...
char I2C_D0D1[10] = "OLED"; //

// STOP Specific ROOM settings for accessories: ///////////////////////////////////////////////////////////////////////////



// GENERAL settings:
STARTUP(WiFi.selectAntenna(ANT_AUTO)); // FAVORITE: continually switches at high speed between antennas
SYSTEM_MODE(AUTOMATIC); // Needed?
SYSTEM_THREAD(ENABLED); // User firmware runs also when not cloud connected. Allows mesh publish & subscribe code to continue even if gateway is not on-line or turned off.

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
char stat_ROOM[40];
char stat_LIGHT[40];
char stat_HEAT[40];
char stat_ALERT[40];

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
Timer kpTimer(200, scan16key);                     // Software Timer to scan every 100 - 200ms = Run the function scan16key()

// *A1: OPTIONAL connector: NONE, DOTSTAR_D, TOUCH2, ... => Currently no specific variables...

// *A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG
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
int CO2_R, CO2_G, CO2_B;   // Colours = To use Pixel as CO2 indicator
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
int getTstatInterval = 30 * 1000; // Sample rate for Tstat
int getTstatLastTime = millis() - getTstatInterval;  // Reset Tstat reading interval to sample immediately at start-up!

// *A7 OP3: Can be used for 2 functions:
// OPTION 1: Door LASER barrier sensor (LDR)
String DOORmovstatus = "Laserbeam doors closed"; // Temporary string
String DOORlightstatus = "Laserbeam lights OFF"; // Temporary string
String laser = "on"; // Initialize the laserbeam to "ON" (=AUTO position) => Can be turned off with function manual()
double DOORLaserLevel = 0; // Initial value of Laser LDR at the door: Lights ON
int DOORlightOnTime = millis(); // Records ON time for the CAB lights
int MaxDOORlightOnTime= 10 * 60 * 1000;// Max ON time for the CAB lights: 10 min? (In case door is left open...)
int getLaserInterval = 2 * 1000;
int getLaserLastTime = millis() - getLaserInterval;
// OPTION 2: PIR2 External dimmer output pin
int DimLightLevel = 0;
int DimTime = 6; // = SECONDS - Change this for slower or faster fade time!
int DimStep = DimTime/0.255;

// *D0 & D1 - I2C: OLED display(s)
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#define OLED_RESET D4// => Originally = D4
Adafruit_SSD1306 display(OLED_RESET);
bool flashdisplay = 1; // Allow (1) or Stop (0) the fast particle messages to show on the OLED display.

// *D3 - RoomSense T-BUS
#include <OneWire.h>
const int oneWirePin = D3;  // D3 = I2C-BUS (Check: 4.7K resistor to Vcc!)
OneWire ds = OneWire(oneWirePin);
// Names of sensors are double variables => can be published as "Particle.variables"
double ROOMTemp1;
double* temps[] = {&ROOMTemp1};
// For time stamp (Faulty sensor reporting via CRC checking):
char crcErrorJSON[128];
int crcErrorCount[sizeof(temps)/sizeof(temps[0])];
uint32_t tmStamp[sizeof(temps)/sizeof(temps[0])];
// For temperature calculations:
int getTemperaturesInterval = 120 * 1000; // Sample rate for temperatures (s): To avoid "nervous" frequent heating ON/OFF switching, set this interval high enough! (>2 min)
int getTemperaturesLastTime = millis() - getTemperaturesInterval;  // Reset heating setting interval to sample immediately at start-up!
double celsius; // Holds the temperature from the array of sensors "per shot"...
double Tout = 18; // Initial temperature setting when OUT of home (After measuring Roomtemp2, Tout is set to safe condens limit)
String outTEMPstatus = "outTEMPstatus?"; // Temporary string

// *D4 - PIXEL-line
#include <neopixel.h>
#define PIXEL_COUNT 50
#define PIXEL_PIN D4
#define PIXEL_TYPE WS2812
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
int rgb[3]; // To store the RGB colour picked from the web page for this controller.

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
double ROOMTemp2;     // Room temperature 2
double ROOMHumi;      // Humidity %
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
  if (strcmp(INT_A0, "TOUCH1") == 0)
  {
    pinMode(kpSCL, OUTPUT);                          // clock output
    pinSetFast(kpSCL);                               // set clock default HIGH (for active LOW setting)

    for (int i=0; i < kpCount; i++)                  // set all data pins as
    pinMode(kpSDO[i], INPUT);                      // input

    //Particle.variable("keypad", (char*)keys);        // expose variable when cloud connected => For debugging only
    kpTimer.start();                                 // start sampling every 100ms
  } // endif ROOM setting "TOUCH1"


  // *A1: OPTIONAL connector: NONE, DOTSTAR_D, TOUCH2, ...
  if (strcmp(OP1_A1, "NONE") == 0)
  {
   // Nothing yet...
  } // endif ROOM setting "NONE"

  if (strcmp(OP1_A1, "DOTSTAR_D") == 0)
  {
   // Nothing yet...
  } // endif ROOM setting "DOTSTAR_D"

  if (strcmp(OP1_A1, "TOUCH2") == 0)
  {
   // Nothing yet...
  } // endif ROOM setting "TOUCH2"


  // *A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG
  if (strcmp(GAS_A2D7, "DUST") == 0)
  {
    pinMode(LEDPin,OUTPUT);
    pinMode(DUSTPin,INPUT_PULLUP); // Dust sensor. Input forced HIGH. <analogread> automatically switches pullup OFF (https://docs.particle.io/reference/firmware/photon/#pinmode-)
    Particle.variable("DUSTlevel", &Dust, DOUBLE);
  } // endif ROOM setting "DUST"

  // *A3 - RoomSense LIGHT
  pinMode(LIGHTpin,INPUT_PULLDOWN); // Light sensor. Input forced LOW. (If no sensor connected: 0)
  Particle.variable("ROOM_Light", &ROOMLightlevel, DOUBLE);

  // *A4 - OP3 => CO2 PWM
  if (strcmp(OP2_A4, "CO2_PWM") == 0)
  {
    Particle.variable("CO2ppm", &CO2ppm, DOUBLE);
  } // endif ROOM setting "CO2_PWM"

  // *A5 - PIR2:
  if (strcmp(OP3_A5, "PIR2") == 0)
  {
    pinMode(PIR2pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
    Particle.variable("PIR2movement", &PIR2movement, DOUBLE);
  } // endif ROOM setting "PIR2"

  // *A6 - OP1 (Not 5V!) => Thermostat
  if (strcmp(OP4_A6, "TSTAT") == 0)
  {
    pinMode(TSTATpin, INPUT_PULLUP); // Thermostat. Input forced HIGH.
  } // endif ROOM setting "TSTAT"

  // *A7 OPTION 1 = LASER light sensor (LDR) = "LaserLDRpin" (for door lights)
  if (strcmp(OP5_A7, "LASERRCV") == 0)
  {
    int LaserLDRpin = A7;
    pinMode(LaserLDRpin, INPUT); // LASER sensor (LDR) input
    // TEMPORARY for testing purpose: Check laserbeam operation!
    Particle.variable("LaserLevel", &DOORLaserLevel, DOUBLE);
  } // endif ROOM setting "LASERRCV"

  // *A7 OPTION 2 = PIR2 External dimmer module command
  if (strcmp(OP5_A7, "DIMMER") == 0)
  {
   int PIR2DIMMERpin = A7;
   pinMode(PIR2DIMMERpin, OUTPUT); // Power DIMMER output
  } // endif ROOM setting "DIMMER"

  // *D0 & D1 - I2C: OLED display(s)
  if (strcmp(I2C_D0D1, "OLED") == 0)
  {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  } // endif ROOM setting "OLED"

  // *D3 - RoomSense T-BUS
  for (int i = 0; i < sizeof(temps)/sizeof(temps[0]); i++)
  {
    tmStamp[i] = Time.now();
  }
  //Particle.variable("CRC_Errors", crcErrorJSON, STRING); // Only needed for debugging: Shows accumulated number of errors...
  Particle.variable("ROOM_Temp1", &ROOMTemp1, DOUBLE);

  // *D4 - PIXEL-line
  //Particle.function("rgb", ledrgb); // Show currently selected colour value from webpage (For debugging only!)
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
  // General commands:
  // 1. Check memory
  freemem = System.freeMemory();// For debugging: Track if memory leak exists...
  memPERCENT = (freemem/51944)*100;

  if(memPERCENT < 50)
  {
    Particle.publish(stat_ALERT, "MEMORY LEAK in controller: Restarting!",60,PRIVATE);
    System.reset();
  }

  // 2. Check if it is day or night:
  checkDAY_NIGHT();

  // 3. REPORT Std Roomsense data 1) To Homebridge, 2) To OLED display:
  if ((millis()-HomebridgeLastTime) > HomebridgeInterval)
  {
    // In "Homebridge" format
    Particle.publish("tvalue", "temperature=" + String(ROOMTemp1,1)); delay(500); // Roomtemperature
    Particle.publish("hvalue", "humidity=" + String(ROOMHumi,0)); delay(500); // Room humidity
    Particle.publish("lvalue", "light=" + String(ROOMLightlevel,0)); delay(500); // Room light

    // For OLED display
    if (strcmp(I2C_D0D1, "OLED") == 0)
    {
      flashdisplay = 0; // Stop showing the fast particle messages on the display
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println(device_name);// Name of the ROOM (Controller)
      display.setTextSize(1);
      display.setCursor(0,20);
      display.println("Temp1 = " + String(ROOMTemp1,0));
      display.println("Humid = " + String(ROOMHumi,0));
      display.println("Light = " + String(ROOMLightlevel,0));
      display.display();
    } // endif ROOM setting "OLED"

    // Optional measurement, when DUST sensor is used.
    if (strcmp(GAS_A2D7, "DUST") == 0)
    {
      // For OLED display
      if (strcmp(I2C_D0D1, "OLED") == 0)
      {
        display.println("Dust  = " + String(Dust,0));
        display.display();
      } // endif ROOM setting "OLED"
    } // endif ROOM setting "DUST"

    // Optional measurement, when CO2 sensor is used.
    if (strcmp(OP2_A4, "CO2_PWM") == 0)
    {
      // In "Homebridge" format
      Particle.publish("co2value", "co2ppm=" + String(CO2ppm,1)); delay(500);// As the Homebridge Particle plugin does not yet allow gas sensors, you must call it a Humidity sensor

      // For OLED display
      if (strcmp(I2C_D0D1, "OLED") == 0)
      {
        display.println("CO2   = " + String(CO2ppm,0));
        display.display();
      } // endif ROOM setting "OLED"
    } // endif ROOM setting "CO2_PWM"

    HomebridgeLastTime = millis(); // Reset reporting timer
  }

  // After half of the HomebridgeInterval, show "flashing messages" again!
  if ((millis()-HomebridgeLastTime) > HomebridgeInterval/2)
  {
   flashdisplay = 1; // Show the fast particle messages again on the display
  }

  // *A0 & D2: INTERFACE connector: 16-key touchpad & OLED display(s)
  if (strcmp(INT_A0, "TOUCH1") == 0)
  {
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

        // Pad 1: Create 16 boolean variables: One of the Keys touched?
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
      } // END if (there was a change on the keypad)
    } // END all pads iterated
  } // endif ROOM setting "TOUCH1"

  // *A2 & D7: ROOMSENSE connector: NONE, GAS, DUST, ...
  if (strcmp(GAS_A2D7, "GAS") == 0)
  {
    // Put here commands for GAS sensor!
  } // endif ROOM setting "GAS_A"

  if (strcmp(GAS_A2D7, "DUST") == 0)
  {
    // * A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG => Used by a DUST sensor...
    if ((millis()-getDustLastTime) > getDustInterval)
    {
      ReadDust();
      // Put eventual extra actions/calculations with DUST here...
      if(Dust > 40 && Dust <= 80)
      {
        Particle.publish(stat_ALERT, "SMOKE or DUST!",60,PRIVATE);
      }

      if(Dust > 80)
      {
        Particle.publish(stat_ALERT, "HEAVY SMOKE or DUST!",60,PRIVATE);
      }
      getDustLastTime = millis();
    }
  } // endif ROOM setting "DUST_A"


  // *A3 - RoomSense LIGHT = Std function!
  if ((millis()-getLightLastTime) > getLightInterval)
  {
    ReadLight();
    // Put eventual extra actions/calculations with LIGHT here...
    getLightLastTime = millis();
  }

  // *A4: OPTIONAL connector:
  if (strcmp(OP2_A4, "CO2_PWM") == 0)
  {
    // *A4 - OP3 => CO2 PWM
    if ((millis()-getCO2LastTime) > getCO2Interval) // Time to update!
    {
      checkCO2levelPWM(A4); // Read pin A4 (via OP connector) => CO2ppm ; Remark: In this function the pin is specified in the loop()

      // CO2 INDICATOR: The bathroom nightLEDs indicate the CO2 level: Red increases, Green decreases...
      CO2_R = constrain((CO2ppm / 5), 0, 255);
      CO2_G = constrain((255 - (CO2ppm / 5)), 0, 255);
      CO2_B = 0;

      /* Voor BandB room: 4 "CO2 indicator LEDs" in badkamer! Tijdelijk uitgeschakeld. Later opnieuw inschakelen...
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
  } // endif time to update!
} // endif ROOM setting "CO2_PWM"


// *A5: OPTIONAL connector: PIR2, Dotstar or LCD-Reset...
if (strcmp(OP3_A5, "PIR2") == 0)
{
  // *A5 - PIR2: Second group of lights
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
} // endif ROOM setting "PIR2"

if (strcmp(OP3_A5, "DOTSTAR_C") == 0)
{
  // Put here Dotstar commands
} // endif ROOM setting "DOTSTAR_C"

if (strcmp(OP3_A5, "RESET") == 0)
{
  // Put here reset commands
} // endif ROOM setting "RESET"




// *A6: OPTIONAL connector: NONE, TSTAT, ...
if (strcmp(OP4_A6, "TSTAT") == 0)
{
  // *A6 - OP1 (Not 5V!) => Thermostat: Reports to HVAC controller if there is heat demand.
  if ((millis()-getTstatLastTime)>getTstatInterval) // This interval can be more frequent, as a thermostat usually has an "hysteresis" of 1°C
  {
    if (digitalRead(TSTATpin) == LOW) // Pin connected to GND = Heat demand!
    {
      tstatTEMPstatus = "Tstat Heat demand"; // For roomsensor area
    }
    else
    {
      tstatTEMPstatus = "No Tstat heat demand"; // For roomsensor area
    }
    Particle.publish(stat_HEAT, tstatTEMPstatus, 60,PRIVATE); delay(500);
    getTstatLastTime = millis(); // Reset the timer
  }
} // endif ROOM setting "TSTAT"






// *A7: OPTIONAL connector: NONE, DIMMER, LASERRCV, ...
if (strcmp(OP5_A7, "LASERRCV") == 0)
{
  // *A7 - LASER sensor (LDR) = "LaserLDRpin"
  if ((millis()-getLaserLastTime) > getLaserInterval) // => It's time to check the LDR signal
  {
    readLaser();

    if (DOORLaserLevel < 500 && DOORmovstatus != "Laser monitored doors open") // Door open => DOORLaserLevel LOW and status changed => lights ON
    {
      DOORmovstatus = "Laser monitored doors open"; Particle.publish(stat_ROOM, DOORmovstatus, 60,PRIVATE); delay(500);
      // Only turn CAB light ON if laser is "on" (enabled). If problems with laser beam, turn laser "off" with function manual("laseroff")...
      if (laser == "on") // Laser barrier is enabled (= Default)
      {
        DOORlightsON(); // Turn door lights ON!
      }
    }

    // If door light ON time is expired, temporarily disable laser and turn door lights OFF. (Door left open!)
    if ((millis()-DOORlightOnTime) >= MaxDOORlightOnTime && DOORmovstatus != "Laser monitored doors closed" && DOORlightstatus != "Door lights OFF") // Doors open and time is up! (To avoid repeating, do it only if lights are ON...)
    {
      DOORlightsOFF();
      laser = "off"; // Disable the laser barrier
    }

    if (DOORLaserLevel >= 500 && DOORmovstatus != "Laser monitored doors closed") // Doors closed => DOORLaserLevel HIGH and status changed => lights OFF
    {
      DOORlightsOFF(); // Turn Laser door lights OFF!
      DOORmovstatus = "Laser monitored doors closed"; Particle.publish(stat_ROOM, DOORmovstatus, 60,PRIVATE); delay(500);
      laser = "on"; // When the laser reaches the LDR, make sure the laserbarrier is enabled!
    }

    getLaserLastTime = millis(); // Reset timer
  }
} // endif ROOM setting "LASERRCV"



// *D3 - T-BUS: Reports to HVAC controller if there is condens danger. (= Std function)
if ((millis()-getTemperaturesLastTime)>getTemperaturesInterval) // To avoid "nervous" frequent switching, set this interval high enough!
{
  getTemperatures(0); // Update all sensor variables of array 0 (DS18B20 type)
  // Actions/calculations with T-BUS output:
  if (ROOMTemp1 < Tout) // Report if room temperature is close to the condensation limit (Tout is a few degrees higher for safety!) Added a max limit for Tout of 16°C...
  {
    outTEMPstatus = "Tout heat demand (Humidity)";
  }
  else
  {
    outTEMPstatus = "No Tout heat demand (Humidity)";
  }
  Particle.publish(stat_HEAT, outTEMPstatus, 60,PRIVATE); delay(500);
  getTemperaturesLastTime = millis(); // Reset the timer
}



// *D5 - RoomSense PIR1 (= Std function)
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



// *D6 - RoomSense TEMP/HUM (= Std function)
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
      TIMEstatus = "It is DAYTIME";
      Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE); delay(500);
    }
  }

  else // If SUNLightlevel <= Nightlevel
  {
    itisnight = 1; // It is night!
    if (TIMEstatus != "It is NIGHT")
    {
      TIMEstatus = "It is NIGHT";
      Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE); delay(500);
    }
  }
}





// * A0-D2-Single-16touch-keypad
void scan16key()
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



// START Specific ROOM settings for 16-key touchpad: "Room-INKOM"////////////////////////////////////////////////////////////

void FunctionKey1() // Default: Turn PIR1 lights ON for the preset time
{
  flashdisplay = 0; // Do not show fast particle messages
  PIR1Lightson();
  PIR1LightONLastTime = millis(); // Reset timer

  Particle.publish("action","Key-1",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("1:PIR1 ON"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Turn PIR1 lights on, whatever the time of the day. They will turn off after the preset time."); display.display();
}

void FunctionKey2() // Default: Force PIR1 lights OFF
{
  flashdisplay = 0; // Do not show fast particle messages
  PIR1Lightsoff();

  Particle.publish("action","Key-2",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("2:PIR1 OFF"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Force PIR1 lights off! They will turn ON normally when it's night and when PIR1 is activated..."); display.display();
}

void FunctionKey3() // Default: Force PIR2 lights ON for the preset time
{
  flashdisplay = 0; // Do not show fast particle messages
  PIR2Lightson();
  PIR2LightONLastTime = millis(); // Reset timer

  Particle.publish("action","Key-3",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("3:PIR2 ON"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Turn PIR2 lights on, whatever the time of the day. They will turn off after the preset time."); display.display();
}

void FunctionKey4() // Default: Force PIR2 lights OFF
{
  flashdisplay = 0; // Do not show fast particle messages
  PIR2Lightsoff();

  Particle.publish("action","Key-4",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("4:PIR2 OFF"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Force PIR2 lights off! They will turn ON normally when it's night and when PIR2 is activated..."); display.display();
}

void FunctionKey5()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-5",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-5"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey6()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-6",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-6"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey7()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-7",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-7"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey8()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-8",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-8"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey9()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-9",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-9"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey10()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-10",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-10"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey11()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-11",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-11"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey12()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-12",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-12"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey13()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-13",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-13"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey14()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-14",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-14"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey15()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-15",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-15"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey16()
{
  flashdisplay = 0; // Do not show fast particle messages
  Particle.publish("action","Key-16",60,PRIVATE); delay(500); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-16"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}
// STOP Specific ROOM settings for 16-key touchpad ///////////////////////////////////////////////////////////////////////////




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
void readLaser() // Check if laser hits LDR or not => DOORLaserLevel => Control door lights
{
  DOORLaserLevel = 0;
  for(int x = 0 ; x < 20 ; x++)
  {
    DOORLaserLevel = DOORLaserLevel + analogRead(A7); // Use A7 directly: You can not create a name for this pin as it is used for 2 functions
  }
  DOORLaserLevel = DOORLaserLevel/20; // Average
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
    snprintf(newAddress, sizeof(newAddress), "0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]); Particle.publish("OneWire", newAddress, 60, PRIVATE); delay(500);

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


// START Specific ROOM settings for LIGHTING: "Room-INKOM"////////////////////////////////////////////////////////////

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

void PIR1demo() // One cycle dimming ON/OFF...
{
  PIR1Lightson();
  delay(5000);
  PIR1Lightsoff();
}

// PIR2 lights CONTROL. 2 variants: 1) If A7 = PIR2 dimmer pin => Send PWM signal from A7; 2) Else: Control POWERPIXELs.
void PIR2Lightson() // Turn second group of lights ON
{
  if (strcmp(OP5_A7, "DIMMER") == 0) // Variant 1: With PWM dimmer
  {
    // Put here the A7 dimmer commands (BandB)
    // Use PIR2DIMMERpin for PWM output
    while (DimLightLevel > 0)
    {
     DimLightLevel--;
     analogWrite(A7, DimLightLevel); // Use A7 directly; You cannot create a name for this pin as it may be used for different functions
     delay(DimStep);
    }
  } // endif ROOM setting "DIMMER"

  else // Variant 2: With Powerpixels
  {
    // Simple switching option:
    //strip.setPixelColor(1, strip.Color(255,255,255)); strip.show(); // Simple switching: R= Second roomlights (SIMPLE SWITCH ON = For testing)
    strip.setPixelColor(1, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Adopt the RGB values set remotely

    // Dimming option:
    //DimUp(1, 0, 255, 5); // Spanlampen boven = PowerPiXel 1 (= second), Pixel #, Start - End, Nr of steps
  } // endif ROOM setting not "DIMMER"

  // Whatever output is used, report PIR2lightstatus:
  PIR2lightstatus = "PIR2 light is ON"; Particle.publish(stat_LIGHT, PIR2lightstatus, 60,PRIVATE); delay(500);
  PIR2light = 1;
}

void PIR2Lightsoff() // Turn second group of lights OFF
{
  if (strcmp(OP5_A7, "DIMMER") == 0) // Variant 1: With PWM dimmer
  {
    // Put here the A7 dimmer commands (BandB)
    // Use PIR2DIMMERpin for PWM output
    while (DimLightLevel < 255)
    {
     DimLightLevel++;
     analogWrite(A7, DimLightLevel); // Use A7 directly; You cannot create a name for this pin as it may be used for different functions
     delay(DimStep);
    }
  } // endif ROOM setting "DIMMER"

  else // Variant 2: With Powerpixels
  {
    // Simple switching option:
    strip.setPixelColor(1, strip.Color(0,0,0)); strip.show(); // R= Second group of lights (SIMPLE SWITCH ON = For testing)

    // Dimming option:
    //DimDown(1, 255, 0, 5); // Spanlampen boven = PowerPiXel 1 (= second), Pixel #, Start - End, Nr of steps
  } // endif ROOM setting not "DIMMER"

  // Whatever output is used, report PIR2lightstatus:
  PIR2lightstatus = "PIR2 light is OFF"; Particle.publish(stat_LIGHT, PIR2lightstatus, 60,PRIVATE); delay(500);
  PIR2light = 0;
}

void PIR2demo() // One cycle dimming ON/OFF...
{
  PIR2Lightson();
  delay(5000);
  PIR2Lightsoff();
}


// door lights: Responding to the LASERRCV signal (A7)
void DOORlightsON() // Turn door lights ON
{
  DOORlightOnTime = millis(); // Restart the door light ON timer. Will turn OFF lights after expiring...
  strip.setPixelColor(3, strip.Color(255,255,255)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power ON
  DOORlightstatus = "Door lights ON"; Particle.publish(stat_LIGHT, DOORlightstatus, 60,PRIVATE); delay(500);
}

void DOORlightsOFF() // Turn door lights (Powerpixel 3) OFF
{
  strip.setPixelColor(3, strip.Color(0,0,0)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power OFF
  DOORlightstatus = "Door lights OFF"; Particle.publish(stat_LIGHT, DOORlightstatus, 60,PRIVATE); delay(500);
}
// STOP Specific ROOM settings for LIGHTING: ///////////////////////////////////////////////////////////////////////////




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

// A. Receive the device NAME from Particle cloud
void DevNamehandler(const char *topic, const char *name)
{
  strncpy(device_name, name, sizeof(device_name)-1); // Store the device name in a string
  snprintf(stat_ROOM, sizeof(str), "Status-ROOM:%s", (const char*)device_name); // Create the Status-ROOM event name with the device name at the end
  snprintf(stat_LIGHT, sizeof(str), "Status-LIGHT:%s", (const char*)device_name); // Create the Status-LIGHT event name with the device name at the end
  snprintf(stat_HEAT, sizeof(str), "Status-HEAT:%s", (const char*)device_name); // Create the Status-HEAT event name with the device name at the end
  snprintf(stat_ALERT, sizeof(str), "Status-ALERT:%s", (const char*)device_name); // Create the Status-ALERT event name with the device name at the end
}

// B. *OLED display: Receiving and displaying the Particle events
void eventDecoder(const char *event, const char *data) // = Called when an event starting with "Status-" prefix is published...
{
  // 1. Create the Name and Subject variables from the messages received:
  char* Name = strtok(strdup(event), ""); // = Type of message
  char* Subject = strtok(strdup(data), ""); // = Message itself
  // Explanation of command "strtok(strdup(event), "")": => Take first string until delimiter = ":" (If there is no : then the full string is copied!)

  if (strcmp(I2C_D0D1, "OLED") == 0 && flashdisplay) // Only show the messages if flashdisplay = 1
  {
    // Display both strings on the OLED display:
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println(Name);
    display.setTextSize(1);
    display.setCursor(0,20);
    display.println(Subject);
    display.display();
  } // endif ROOM setting "OLED"


  // 2. Receive the SUNlight level (lux) from the messages starting with "SUN:"
  char* Sun = strtok(strdup(data), ":"); // Take first string until delimiter = ":" (If there is no : then the full string is copied!)

  // Decode variables: If "Subject" starts with a 3-character string followed by ":" => Create variable names, depending on the "Subject" name...
  // 1) SUNLightlevel: To be used to enable exterior lights.
  if (strncmp(Sun, "SUN", strlen("SUN")) == 0) // String = SUN
  {
    SUNLightlevel = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable
  }

  free(Name); free(Subject); free(Sun); // ATTENTION:"free" these variables every time to avoid memory leak!
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

    // Room_settings
    Particle.publish("Setting I2C_D0D1", I2C_D0D1,60,PRIVATE); delay(1000);
    Particle.publish("Setting OP5_A7", OP5_A7,60,PRIVATE); delay(1000);
    Particle.publish("Setting OP4_A6", OP4_A6,60,PRIVATE); delay(1000);
    Particle.publish("Setting OP3_A5", OP3_A5,60,PRIVATE); delay(1000);
    Particle.publish("Setting OP2_A4", OP2_A4,60,PRIVATE); delay(1000);
    Particle.publish("Setting GAS_A2D7", GAS_A2D7,60,PRIVATE); delay(1000);
    Particle.publish("Setting OP1_A1", OP1_A1,60,PRIVATE); delay(1000);
    Particle.publish("Setting INT_A0", INT_A0,60,PRIVATE); delay(1000);

    // ROOMSENSE box std functions
    sprintf(str, "Room Light: %2.0f",ROOMLightlevel); Particle.publish(stat_LIGHT, str,60,PRIVATE); delay(500);
    sprintf(str, "Room Temp1: %2.1f",ROOMTemp1); Particle.publish(stat_HEAT, str,60,PRIVATE); delay(500);
    sprintf(str, "Room Temp2+Humid+Dew+Tout:%2.1f+%2.0f+%2.1f+%2.1f",ROOMTemp2,ROOMHumi,ROOMTdf,ROOMTout); Particle.publish(stat_HEAT, str,60,PRIVATE); delay(500);
    Particle.publish(stat_HEAT, outTEMPstatus, 60,PRIVATE); delay(500);
    Particle.publish(stat_HEAT, tstatTEMPstatus, 60,PRIVATE); delay(500);

    // ROOMSENSE box optional CO2
    if (strcmp(OP2_A4, "CO2_PWM") == 0)
    {
      sprintf(str, "Room CO2 x100: %2.0f",CO2ppm); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    } // endif ROOM setting "CO2_PWM"

    // ROOMSENSE box optional DUST
    if (strcmp(GAS_A2D7, "DUST") == 0)
    {
      sprintf(str, "Room DUST Pct: %2.0f",Dust); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    } // endif ROOM setting "DUST"

    // All RGB related lights: Selected RGB colour
    sprintf(str, "Colour:%d-%d-%d",rgb[0],rgb[1],rgb[2]); Particle.publish(stat_LIGHT, str,60,PRIVATE); delay(500);

    // PIR1 Lights (= STD)
    sprintf(str, "PIR1movement: %2.0f",PIR1movement); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
    Particle.publish(stat_ROOM, PIR1movstatus, 60,PRIVATE); delay(500);
    Particle.publish(stat_LIGHT, PIR1lightstatus, 60,PRIVATE); delay(500);

    // PIR2 Lights (OPTional)
    if (strcmp(OP3_A5, "PIR2") == 0)
    {
      sprintf(str, "PIR2movement: %2.0f",PIR2movement); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
      Particle.publish(stat_ROOM, PIR2movstatus, 60,PRIVATE); delay(500);
      Particle.publish(stat_LIGHT, PIR2lightstatus, 60,PRIVATE); delay(500);
    } // endif ROOM setting "PIR2"

    // door Laser barrier & Lights
    if (strcmp(OP5_A7, "LASERRCV") == 0)
    {
      sprintf(str, "Laser LDR: %2.0f",DOORLaserLevel); Particle.publish(stat_ROOM, str,60,PRIVATE); delay(500);
      Particle.publish(stat_ROOM, DOORmovstatus, 60,PRIVATE); delay(500);
      Particle.publish(stat_LIGHT, DOORlightstatus, 60,PRIVATE); delay(500);
    } // endif ROOM setting "LASERRCV"

    // 1-wire sensors
    discoverOneWireDevices();

    return 1000;
  }

  if((command == "pir1on") || (command == "Lights=1}")) // Second condition is for Homebridge
  {
    PIR1Lightson();
    PIR1LightONLastTime = millis(); // Reset turn OFF interval
    return 100;
  }

  if((command == "pir1off") || (command == "Lights=0}")) // Second condition is for Homebridge
  {
    PIR1Lightsoff();
    return 1;
  }

  if(command == "pir1demo") //
  {
    PIR1demo();
    return 1;
  }

  if (strcmp(OP3_A5, "PIR2") == 0)
  {
    if((command == "pir2on") || (command == "EXT1=1}")) // Second condition is for Homebridge
    {
      PIR2Lightson();
      PIR2LightONLastTime = millis(); // Reset turn OFF interval
      return 200;
    }

    if((command == "pir2off") || (command == "EXT1=0}")) // Second condition is for Homebridge
    {
      PIR2Lightsoff();
      return 2;
    }

    if(command == "pir2demo") //
    {
      PIR2demo();
      return 1;
    }
  } // endif ROOM setting "PIR2"

  if (strcmp(OP5_A7, "LASERRCV") == 0) // Only if a Laserbeam is installed
  {

    if(command == "laseron") // Enable the door laser barrier and let CAB lights automatically react to it
    {
      laser = "on";
      return 1;
    }

    if(command == "laseroff") // Disable the door laser barrier and turn CAB lights OFF
    {
      laser = "off";
      DOORlightsOFF();
      return 0;
    }

    if(command == "dooron")
    {
      DOORlightsON();
      return 300;
    }

    if(command == "dooroff")
    {
      DOORlightsOFF();
      return 3;
    }
  } // endif ROOM setting "LASERRCV"

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
