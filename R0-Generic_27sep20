// ROOM_Generic_24sep21.ino = Generic ROOM sketch - Installed on all roomcontrollers at Filip's new home + in the common WebIDE account with MakerKen.

// 27sep21: Solved issue of device_name not being caught: see https://community.particle.io/t/surprising-issue-after-flashing-new-sketch-to-my-10-roomcontrollers/61194/5
// 26sep21: Removed unnecessary parameters ",60,PRIVATE" from all messages and removed library "PublishQueueAsyncRK.cpp", using only standard publish messages.
// 24sep21: Reduced publish frequency with factor 10
// 20aug20: Reduced reserved space for all string variables from 622 to 400 bytes (low memory)
// 10may20: Changed the published JSON string to Ken's new method with "Generate.html". Blanked out the previous line. Google logging script must also adapt!
// 9feb20: Made a Particle.variable "JSON_config" with JSON string of "config" data.
//                                      ISSUE: The Dallas sensor ID does not show yet!
// 8feb20: Made a Particle.variable  "JSON_commands" with JSON string of all commands for Particle.command "Manual": to construct dropdown list in JS App.
// 31jan20: Added 2 lines to setup() to make sure Mov1 & 2 lights are OFF initially.
// 12nov19: After upgrading system OS to 1.4.2, PietteTech_DHT library created RED Panic14 error!
//  - Made temporary sketch without that library.
//  => Later, flash this sketch again with OK PietteTech_DHT library (Scruffr!)
// 14nov19: Updated with ScruffR's 3 corrections: https://community.particle.io/t/piettetech-dht-causing-sos-14-flashes-on-v1-2-1-rc-3/50583/31?u=fidel
//
// To do:
// - Solve issue: "1-wire bus Scanner" function: Not always working. Depends on accessories selected...
// - Integrate second OLED display: Show KEY instructions!
// - Test DOTSTAR-line op A1 (Data) + A5 (Clock)
// - Test GAS and FLAME sensor on pins D7&A2 instead of DUST sensor.
//
// Purpose: "Puts life" in a PhotoniX shield with it's accessories connected via the default ports (Details below...)
//
// ---------------------------------------
// PhotoniX shield v.3.0	I/O connections: (* = used)
//
// *D0/D1 - COM1/2 (I2C-SDA & SCL) = OLED display, I/O expander, Sunlight sensor (TSL2561)...
// *D2 - COM3 = 16-key touchpad (CLK) or TOUCH-COMMON
// *D3 - RoomSense T-BUS
// *D4 - OP5 = PIXEL-line
// *D5 - RoomSense MOV1
// *D6 - RoomSense TEMP/HUM
// *D7 - RoomSense GAS-DIG or DUST sensor (LED transmitter)
// *A0 - COM4 = 16-key touchpad (DATA1) = TOUCH-1
// *A1 - COM5 = DOTSTAR-line (Data) or 16-key touchpad (DATA2) = TOUCH-2
// *A2 - RoomSense GAS-ANA or DUST sensor (LED receiver)
// *A3 - RoomSense LIGHT => Put 22k series resistor with LDR (Not 5V tolerant!)
// *A4 - OP3 = PWM input for CO2
// *A5 - OP4 = MOV2 (Second light group), LCD-Reset or DOTSTAR-line (Clock)
// *A6 - OP1 (Not 5V!) = Thermostat switch to GND
// *A7 - OP2 = MOV2 operated dimmer pin or Alert light sensor (LDR) input.
//  TX/RX - COM6/7 = Serial comms
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
// - The standard environmental data are measured: Room temperature (2sensors), humidity and light value.
// - Optional data: D7 + A2 = Gas or Dust (smoke) sensor.
//
// OP connector (Actual use: see above table)
// examples:
// - A4 Option: Room CO2 ppm level (Based on "CUBIC PWM CO2 Monitor.ino" =>  Duty Cycle analyzer for PWM CO2 sensors)
// - A5 Option: MOV2 movement sensor: Triggers a second group of lights
// - A6 Option: Room thermostat: Control heating in "Home" mode.
// - A7 Option1: Lighting control with PWM signal. In BandB: Two 300 mA LEDs in series, controlled by a dimmer module, triggered by MOV2 sensor.
// - A7 Option2: Alert (laser) barrier detects opening STOREs (doors, drawers etc...) For example: A light sensor triggers the STORE lights ON if under a set value.
//
//
// T-BUS (D3)
// - Precision temperature sensor(s). The reading of this sensor is used to recalibrate the second one, less accurate sensor.
// - 1-wire address scanner: List all DS18B20 sensors!
// - Control heating in "HOME" mode: Follow the (manual) thermostat setting + Condens protection like in "OUT" mode.
// - Control heating in "OUT" mode (or for Condens protection in "Home" mode)
//   Follow Tout, the condens lower limit (Dewpoint +2°C), but limit the room temperature to 16°C (It is very seldom that a higher temp will be needed to protect against condense...)
//   => Use "Hysteresis" or longer getTemperaturesInterval (600s) to avoid too frequent switching...
// - A message is sent from IFTTT when the room temperature is close to the Dewpoint.
//
// Day/Night signal: Catching "SUNlight" value broadcast by another controller...
//
// Pixel lighting control (D4 - PIXEL-line)
// 1) MOV1 & MOV2 sensors: Control a chain of PowerPixels:
// - You can set the RGB values with the "colour picker" for that room.
// - Set bedTime / wakeup status from command manual() to keep some lights dim or off during sleep! (MOV1, MOV2 and STORE lights)
// 2) RGB control with a "Picker" from a web form:
//    This was made by Particle community member @makerken: https://community.particle.io/t/simple-rgb-led-control-from-a-web-post-command/36520
//    Advice: Put all PowerPixels first in the string (as LED 0) and then eventually a LED strip.
//    Reason: If one of the strip LEDs fails, the first LED (PowerPiXel) will still keep it's position...
//
// 16-key touchpad:
// - Used to select different modes for lighting, Data Reporting etc...
//
// OLED display:
// - Combined with 16-key touchpad.
// - It displays Particle cloud "Status" messages and feedback / instructions of the 16-key touchpad.
//   It alternates between this display and a display showing the main Room data.
//
// Publishing:
// - Publishes all variables and status topics in one string
//
// Asynchronous publishing:
// - We use the library "PublishQueueAsyncRK" made by Particle Community's @Rikkas.
// => This allows sending messages without adding delay() commands. Normally a Particle device refuses to publish these if the pace exceeeds 1/s.
//    With this library, messages are "Queued" and published "asynchronously" at a rate of one/s.
//    As long as they are not sent, they are kept in a retained memory buffer...
//


// START Specific ROOM settings 1 (for accessories): "Room-INKOM2"////////////////////////////////////////////////////////////

// *D3: ROOMSENSE connector: Select DS18B20 temperature sensors in use by uncommenting line(s):
//byte addrs0[1][8] = {{0x28,0xFF,0x43,0x6D,0x33,0x17,0x04,0x3B}};  // INKOM room: New Round RoomSenseBoX
//byte addrs0[1][8] = {{0x28,0xFF,0xBB,0x7C,0x33,0x17,0x04,0x14}};  // BADK room: New Round RoomSenseBoX
//byte addrs0[1][8] = {{0x28,0xFF,0x3E,0x41,0x33,0x17,0x04,0xD8}};  // BandB room:  Old Square RoomSenseBoX
//byte addrs0[1][8] = {{0x28,0xFF,0x2E,0x82,0xA7,0x15,0x01,0x0D}};  // WASPL room: New Round RoomSenseBoX
byte addrs0[1][8] = {{0x28,0xFF,0x1A,0x3A,0x33,0x17,0x04,0x03}};  // TEST Photon @Filip's current home (New Round RoomSenseBoX)
//byte addrs0[1][8] = {{0x28,0xFF,0xFB,0x70,0x33,0x17,0x04,0xEF}};  // TEST Photon @MakerKen in Canada (Old Square RoomSenseBoX)

// *A0 & D2: INTERFACE connector: NONE, TOUCH1, ...
char INT_A0[10] = "TOUCH1";
// *A1: OPTIONAL connector: NONE, DOTSTAR_D, TOUCH2, ...
char OP1_A1[10] = "NONE";
// *A2 & D7: ROOMSENSE connector: NONE, GAS, DUST, ...
char GAS_A2D7[10] = "DUST";
// *A4: OPTIONAL connector: NONE, CO2_PWM, ...
char OP2_A4[10] = "CO2_PWM";
// *A5: OPTIONAL connector: NONE, MOV2 (w PIXELs), DOTSTAR_C, RESET (LCD), ...
char OP3_A5[10] = "MOV2";
// *A6: OPTIONAL connector: NONE, TSTAT...
char OP4_A6[10] = "TSTAT";
// *A7: OPTIONAL connector: NONE, (MOV2) DIMMER, ALERTRCV, ...
char OP5_A7[10] = "ALERTRCV";
// *D0/D1: I2C connector: NONE, OLED, 16P_EXP, ...
char I2C_D0D1[10] = "OLED";

// Attention: Also check and eventually adapt the Particle.variable "JSON_commands" in the setup() section to the parameters used in particle.function "manual" at the bottom of this sketch!

// STOP Specific ROOM settings 1 (for accessories): "Room-INKOM2"///////////////////////////////////////////////////////////////////////////



// GENERAL settings:
STARTUP(WiFi.selectAntenna(ANT_AUTO)); // FAVORITE: continually switches at high speed between antennas
SYSTEM_MODE(AUTOMATIC); // Needed?
SYSTEM_THREAD(ENABLED); // User firmware runs also when not cloud connected. Allows mesh publish & subscribe code to continue even if gateway is not on-line or turned off.

// WiFi reception
float wifiPERCENT;  // New 10may20! (changed from double to float)
float wifiQUAL;     // New 10may20! (changed from double to float)

// Memory monitoring
float freemem;
float memPERCENT;

// Strings for publishing
char str[255]; // Temporary string for all messages
char JSON_init[400]; // Publish all initialization parameters in one string (Max 622)
char JSON_commands[400]; // Publish all Manual() function commands in one string (Max 622)
char JSON_status[400]; // Publish all status variables in one string (Max 622)

// For storing Particle device NAME in event name groups:
char device_name[32] = "";
char stat_ROOM[40];
char stat_LIGHT[40];
char stat_HEAT[40];
char stat_ALERT[40];

// Homebridge reporting
int HomebridgeInterval = 120 * 1000;
int HomebridgeLastTime = millis() - HomebridgeInterval;

// Sunlight level (Broadcasted by another Photon)
double SUNLightlevel = 0; // Sunlight level from the (external) SOLAR sensor => Initially set to NIGHT- or DAYtime level (Value will be set from a "broadcast message" from a "daylight controller.
int Nightlevel = 150; // Below this SUNLightlevel it is NIGHT! (broadcasted by another Photon running "I2C-Daylightsensor.ino")
boolean ItIsNight = 0; // This will be set to 1 or 0, depending on "checkDAY_NIGHT()" function. (Initialized as DAY)
String TIMEstatus = "Initialized as DAY!"; // Temporary string
boolean BedTime = 0;// BedTime status: Remotely created in function manual()


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
int getDustInterval = 120 * 1000;
int getDustLastTime = millis() - getDustInterval;

// *A3 - RoomSense LIGHT
int LIGHTpin = A3;
double ROOMLightlevel;// Double to be published!
int getLightInterval = 61 * 1000;
int getLightLastTime = millis() - getLightInterval;

// *A4 - OP3 => CO2 PWM
// In this function pin A4 is specified in the loop()
double pulseTime, CO2ppm;  // Use floating variables for calculations
int CO2_R, CO2_G, CO2_B;   // Colours = To use Pixel as CO2 indicator
int getCO2Interval = 62 * 1000; // Sample rate for CO2
int getCO2LastTime = millis() - getCO2Interval;

// *A5 - MOV2: Second light group
int MOV2pin = A5;
int MOV2counter = 0; // Counts MOV2 triggers every second
int MOV2counterLastTime;
double MOV2movement = 0; // Total movement over last 5 minutes
int MOV2totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter (5 min like Atomiot interval)
int MOV2totalLastTime;
String MOV2movstatus = "MOV2 empty"; // Initial setting
boolean MOV2occup = 0; // Is this occupied?
String MOV2lightstatus = "MOV2 light is OFF"; // Initial setting
boolean MOV2light = 0; // Is this light ON?
int MOV2LightONtime = 5 * 60 * 1000; // How long should the lights stay ON? (5 min)
int MOV2LightONLastTime;

// *A6 - OP1 (Not 5V!) => Thermostat
int TSTATpin = A6;
String tstatTEMPstatus = "No Tstat heat demand"; // Temporary string
boolean TSTATon = 0;// Is the TSTAT ON?
int getTstatInterval = 63 * 1000; // Sample rate for Tstat
int getTstatLastTime = millis() - getTstatInterval;  // Reset Tstat reading interval to sample immediately at start-up!

// *A7 OP3: Can be used for 2 functions:

// OPTION 1: STORE Alert (ex: laser barrier) sensor (LDR)
String STOREmovstatus = "Alert monitored STOREs closed"; // Temporary string
boolean STOREalert = 0;          // Is STORE alert ON?
String STORElightstatus = "STORE lights OFF"; // Temporary string
boolean STORElight = 0;          // Is STORE alert LIGHT ON?
String Alert = "on"; // Initialize the Alertbeam to "ON" (=AUTO position) => Can be turned off with function manual()
double STOREAlertLevel = 0; // Initial value of Alert LDR at the STORE: Lights ON
int STORElightOnLevel = 500; // Over this Alert level: Lights turn OFF
int STORElightOnTime = millis(); // Records ON time for the CAB lights
int MaxSTORElightOnTime= 10 * 60 * 1000;// Max ON time for the CAB lights: 10 min? (In case STORE is left open...)
int getAlertInterval = 20 * 1000;
int getAlertLastTime = millis() - getAlertInterval;

// OPTION 2: MOV2 External dimmer output pin
int DimLightLevel = 0;
int DimTime = 6; // = SECONDS - Change this for slower or faster fade time!
int DimStep = DimTime/0.255;

// *D0 & D1 - I2C: OLED display(s)
//#include "Adafruit_GFX.h" => FD: Removed temporarily because it gave compilation errors!
#include "Adafruit_SSD1306.h"
#define OLED_RESET D4// => Originally = D4
Adafruit_SSD1306 display(OLED_RESET);
int Displaymode = 0; // Initially: Mode 0 = Show the fast particle messages on the OLED display.
bool Displayfreeze = 0; // Initially: Allow display to switch automatically
int FreezeInterval = 20 * 1000; // Keep the selected display for this minimum time
int FreezeLastTime = millis() - FreezeInterval;

// *D3 - RoomSense T-BUS
#include <OneWire.h>
const int oneWirePin = D3;  // D3 = I2C-BUS (Check: 4.7K pull-up resistor to Vcc!)
OneWire ds = OneWire(oneWirePin);
// Names of sensors are double variables => can be published as "Particle.variables"
double ROOMTemp1;
double* temps[] = {&ROOMTemp1};
// For time stamp (Faulty sensor reporting via CRC checking):
char crcErrorJSON[128];
int crcErrorCount[sizeof(temps)/sizeof(temps[0])];
uint32_t tmStamp[sizeof(temps)/sizeof(temps[0])];
// For temperature calculations:
int getTemperaturesInterval = 5 * 60 * 1000; // Sample rate for temperatures (s): To avoid "nervous" frequent heating ON/OFF switching, set this interval high enough! (>2 min)
int getTemperaturesLastTime = millis() - getTemperaturesInterval;  // Reset heating setting interval to sample immediately at start-up!
double celsius; // Holds the temperature from the array of sensors "per shot"...
double Tout = 18; // Initial temperature setting when OUT of home (After measuring Roomtemp2, Tout is set to safe condens limit)
String outTEMPstatus = "outTEMPstatus?"; // Temporary string
boolean TdfALERT = 0; // Condens alert ON

// *D4 - PIXEL-line
#include <neopixel.h>
#define PIXEL_COUNT 50
#define PIXEL_PIN D4
#define PIXEL_TYPE WS2812
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
int rgb[3]; // To STORE the RGB colour picked from the web page for this controller.

// *D5 - RoomSense MOV1: Entrance & Staircase
int MOV1pin = D5;
int MOV1counter = 0; // Counts MOV1 triggers every second
int MOV1counterLastTime;
double MOV1movement = 0; // Total movement over last 5 minutes
int MOV1totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter (5 min like Atomiot interval)
int MOV1totalLastTime;
String MOV1movstatus = "MOV1 empty"; // Initial setting
boolean MOV1occup = 0; // Is this occupied?
String MOV1lightstatus = "MOV1 light is OFF"; // Initial setting
boolean MOV1light = 0; // Is this light ON?
int MOV1LightONtime = 5 * 60 * 1000; // How long should the lights stay ON? (2 min)
int MOV1LightONLastTime;

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

// All variables which store millis() should be of type "uint32_t":
uint32_t getTempHumInterval = 64 * 1000; // Minimum 2s!
uint32_t getTempHumLastTime = millis() - getTempHumInterval;




void setup()
{
  // GENERAL Settings:
  Time.zone(+1); // Set clock to Belgium time

  // GENERAL Particle functions:
  Particle.function("Manual", manual);

  // GENERAL Particle variables:
  Particle.variable("JSON_status", JSON_status, STRING); // Contains all data to be monitored

  // Publish a JSON_init string with all initialization parameters selected on top of this sketch:
  snprintf(JSON_init,600,"{\"SensorID\":%s,\"INT_A0\":%s,\"OP1_A1\":%s,\"GAS_A2D7\":%s,\"OP2_A4\":%s,\"OP3_A5\":%s,\"OP4_A6\":%s,\"OP5_A7\":%s,\"I2C_D0D1\":%s}",addrs0[0],INT_A0,OP1_A1,GAS_A2D7,OP2_A4,OP3_A5,OP4_A6,OP5_A7,I2C_D0D1);
  Particle.variable("Initialization", JSON_init, STRING); // Contains all commands, available for this controller

  // Publish a JSON_commands string with all available commands in function "manual":
  snprintf(JSON_commands,600,"{\"1\":%s,\"2\":%s,\"3\":%s,\"4\":%s,\"5\":%s,\"6\":%s,\"7\":%s\"8\":%s,\"9\":%s,\"10\":%s,\"11\":%s,\"12\":%s,\"13\":%s,\"14\":%s,\"15\":%s,\"16\":%s,\"17\":%s,\"18\":%s,\"19\":%s}","reportheat","reportlight","reportroom","reportsensors","mov1on","mov1off","mov1demo","mov2on","mov2off","mov2demo","alerton","alertoff","alertlighton","alertlightoff","night","day","bedtime","wakeup","reset");
  Particle.variable("Roomcommands", JSON_commands, STRING); // Contains all commands, available for this controller

  // Initialize Particle subscribe functions:
  // Catching private events
  Particle.subscribe("Status-", EventDecoder, MY_DEVICES); // Listening for the event "Status-*" and, when it receives it, it will run the function EventDecoder()
  // Catching it's own device NAME
  Particle.subscribe("particle/device/name", DevNamereceiver); // Listening for the device name...
  delay(1000);

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

  // *A5 - MOV2:
  if (strcmp(OP3_A5, "MOV2") == 0)
  {
    pinMode(MOV2pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
    Particle.variable("MOV2movement", &MOV2movement, DOUBLE);
    MOV2Lightsoff(); // Make sure the lights turn OFF initially.
  } // endif ROOM setting "MOV2"

  // *A6 - OP1 (Not 5V!) => Thermostat
  if (strcmp(OP4_A6, "TSTAT") == 0)
  {
    pinMode(TSTATpin, INPUT_PULLUP); // Thermostat. Input forced HIGH.
  } // endif ROOM setting "TSTAT"

  // *A7 OPTION 1 = Alert light sensor (LDR) = "AlertLDRpin" (for STORE lights)
  if (strcmp(OP5_A7, "ALERTRCV") == 0)
  {
    int AlertLDRpin = A7;
    pinMode(AlertLDRpin, INPUT); // Alert sensor (LDR) input
    // TEMPORARY for testing purpose: Check Alertbeam operation!
    Particle.variable("AlertLevel", &STOREAlertLevel, DOUBLE);
  } // endif ROOM setting "ALERTRCV"

  // *A7 OPTION 2 = MOV2 External dimmer module command
  if (strcmp(OP5_A7, "DIMMER") == 0)
  {
    int MOV2DIMMERpin = A7;
    pinMode(MOV2DIMMERpin, OUTPUT); // Power DIMMER output
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
  Particle.function("rgb", ledrgb); // Show currently selected colour value from webpage (For debugging only!)
  strip.begin(); strip.show(); // Initialize all pixels to 'off'

  // Initialize RGB color, until "mobile color picker" can be used;
  // ATTENTION: This MUST be in setup() !!!
  rgb[0] = 255; // Red      (0-255)
  rgb[1] = 255; // Green    (0-255)
  rgb[2] = 255;  // Blue     (0-255)

  // *D5 - RoomSense MOV1 (Default)
  pinMode(MOV1pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
  Particle.variable("MOV1movement", &MOV1movement, DOUBLE);
  MOV1Lightsoff(); // Make sure the lights turn OFF initially.

  // *D6 - RoomSense TEMP/HUM
  Particle.variable("ROOM_Temp2", ROOMTemp2);
  Particle.variable("ROOM_Humid",ROOMHumi);
  Particle.variable("ROOM_Tout", ROOMTout);
  DHT.begin();
}






void loop()
{
  // Catching device_name: (Counter-measure for issue of not catching device_name)
  if (!strlen(device_name)) // If the variable has not received contents...
      Particle.publish("particle/device/name"); // Ask the cloud (once) to send the device NAME!

  // General commands:
  // 1a. Check memory
  freemem = System.freeMemory();// For debugging: Track if memory leak exists...
  memPERCENT = (freemem/82944)*100; // Max 82944 kb

  if(memPERCENT < 30)
  {
    Particle.publish(stat_ALERT, "MEMORY LEAK in controller: Restarting!");
    System.reset();
  }

  // 1b. Check WiFi reception => New 10may20! Makes the 16-key pad crazy! Unpower to reset!
  // NEW version:
    //WiFiSignal sig = WiFi.RSSI();
    //wifiPERCENT = sig.getStrength();
    //wifiQUAL = sig.getQuality();

  // 2. Check if it is day or night:
  checkDAY_NIGHT();

  // 3. REPORT Std Roomsense data
  // 1) To Homebridge:
  if ((millis()-HomebridgeLastTime) > HomebridgeInterval)
  {
    Particle.publish("tvalue", "temperature=" + String(ROOMTemp1,1)); // Roomtemperature
    Particle.publish("hvalue", "humidity=" + String(ROOMHumi,0)); // Room humidity
    Particle.publish("lvalue", "light=" + String(ROOMLightlevel,0)); // Room light

    // Only when DUST sensor is used.
    if (strcmp(GAS_A2D7, "DUST") == 0)
    {
      Particle.publish("dvalue", "dust=" + String(Dust,0)); // As the Homebridge Particle plugin does not yet allow gas sensors, you must call it a Humidity sensor
    } // endif ROOM setting "DUST"

    // Only when CO2 sensor is used.
    if (strcmp(OP2_A4, "CO2_PWM") == 0)
    {
      Particle.publish("cvalue", "co2=" + String(CO2ppm,1)); // As the Homebridge Particle plugin does not yet allow gas sensors, you must call it a Humidity sensor
    } // endif ROOM setting "CO2_PWM"
    HomebridgeLastTime = millis(); // Reset reporting timer

    // Put OLED display in Mode 1 to show these variables also
    if (!Displayfreeze)
    {
      Displaymode = 1;
    }

    // NEW 10may20: Store the colour array values in simple variables:
    int Rval = rgb[0];
    int Gval = rgb[1];
    int Bval = rgb[2];

    // Update the System report JSON string:
    //snprintf(JSON_status,400,"{\"Strength\":%.0f,\"Quality\":%.0f,\"FreeMem\":%.0f,\"Temp1\":%.1f,\"Temp2\":%.1f,\"Humi\":%.0f,\"Dew\":%.0f,\"DewAlert\":%d,\"Light\":%.0f,\"Dust\":%.0f,\"CO2\":%.0f,\"TSTATon\":%d,\"MOV1\":%.0f,\"MOV1light\":%d,\"MOV2\":%.0f,\"MOV2light\":%d,\"BEAMalert\":%d,\"BEAMlight\":%d,\"SUNLight\":%.0f,\"Night\":%d,\"Bed\":%d,\"R\":%d,\"G\":%d,\"B\":%d}",WiFi.RSSI().getStrength(),WiFi.RSSI().getQuality(),System.freeMemory()/83200.0*100,ROOMTemp1,ROOMTemp2,ROOMHumi,ROOMTdf,TdfALERT,ROOMLightlevel,Dust,CO2ppm,TSTATon,MOV1movement,MOV1light,MOV2movement,MOV2light,STOREalert,STORElight,SUNLightlevel,ItIsNight,BedTime,rgb[0],rgb[1],rgb[2]);

    // NEW 10may20: Ken's NEW JSON:
    snprintf(JSON_status, 622, "{"
    	"\"a\":%.0f,"
    	"\"b\":%.0f,"
    	"\"c\":%.0f,"
    	"\"d\":%.0f,"
    	"\"e\":%.0f,"
    	"\"f\":%.0f,"
    	"\"g\":%.1f,"
    	"\"h\":%.1f,"
    	"\"i\":%.0f,"
    	"\"j\":%.0f,"
    	"\"k\":%d,"
    	"\"l\":%d,"
    	"\"m\":%d,"
    	"\"n\":%d,"
    	"\"o\":%d,"
    	"\"p\":%d,"
    	"\"q\":%d,"
    	"\"r\":%d,"
    	"\"s\":%d,"
    	"\"t\":%d,"
    	"\"u\":%d,"
    	"\"v\":%.0f,"
    	"\"w\":%.0f,"
    	"\"x\":%.0f}",
    	CO2ppm,
    	Dust,
    	ROOMTdf,
    	ROOMHumi,
    	ROOMLightlevel,
    	SUNLightlevel,
    	ROOMTemp1,
    	ROOMTemp2,
    	MOV1movement,
    	MOV2movement,
    	TdfALERT,
    	TSTATon,
    	MOV1light,
    	MOV2light,
    	STOREalert,
    	STORElight,
    	ItIsNight,
    	BedTime,
    	Rval,
    	Gval,
    	Bval,
    	WiFi.RSSI().getStrength(),
    	WiFi.RSSI().getQuality(),
    	memPERCENT
    );


    //Particle.publish(stat_ROOM, JSON_status); // For debugging. Can be published with manual()
  }

  // 2) To OLED display
  if (strcmp(I2C_D0D1, "OLED") == 0 && Displaymode == 1 && !Displayfreeze) // Only show the messages if Displaymode = 1
  {
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
    // Only when DUST sensor is used.
    if (strcmp(GAS_A2D7, "DUST") == 0)
    {
      display.println("Dust  = " + String(Dust,0));
    } // endif ROOM setting "DUST"
    // Only when CO2 sensor is used.
    if (strcmp(OP2_A4, "CO2_PWM") == 0)
    {
      display.println("CO2   = " + String(CO2ppm,0));
    } // endif ROOM setting "CO2_PWM"
    display.display(); // Show it once...
    Displayfreeze = 1; FreezeLastTime = millis(); // ... and then freeze display!
  } // endif ROOM setting "OLED"

  // After half of the HomebridgeInterval, show "flashing messages" again!
  if ((millis()-HomebridgeLastTime) > HomebridgeInterval/2 && !Displayfreeze)
  {
    Displaymode = 0; // Show the fast particle messages on the display
  }

  // After FreezeInterval, unfreeze the display again, if frozen!
  if ((millis()-FreezeLastTime) > FreezeInterval && Displayfreeze)
  {
    Displayfreeze = 0; // Allow displaying the fast particle messages again on the display
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
        Particle.publish(stat_ALERT, "SMOKE or DUST!");
      }

      if(Dust > 80)
      {
        Particle.publish(stat_ALERT, "HEAVY SMOKE or DUST!");
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

    // CO2 Alert:
    if (CO2ppm > 800) // CO2 level high!
    {
      Particle.publish(stat_ROOM, "Room CO2 ppm HIGH");
    }

    if (CO2ppm > 1800) // CO2 level too high!
    {
      Particle.publish(stat_ROOM, "Room CO2 ppm too HIGH");
    }

    getCO2LastTime = millis(); // Reset timer
  } // endif time to update!
} // endif ROOM setting "CO2_PWM"


// *A5: OPTIONAL connector: MOV2, Dotstar or LCD-Reset...
if (strcmp(OP3_A5, "MOV2") == 0)
{
  // *A5 - MOV2: Second group of lights
  if (digitalRead(MOV2pin) == HIGH) // Motion detected
  {
    if ((millis()-MOV2counterLastTime) > 1000)
    {
      MOV2counter = MOV2counter + 1;
      MOV2counterLastTime = millis(); // Reset 1 minute period
    }
    MOV2LightONLastTime = millis(); // At each PIR trigger: reset ROOMlights turn OFF interval

    if (!MOV2occup)  // Not occupied
    {
      MOV2movstatus = "MOV2 in use"; Particle.publish(stat_ROOM, MOV2movstatus);
      MOV2occup = 1;

      if (ItIsNight && !MOV2light)
      {
        MOV2Lightson();
      }
    }
  }

  else // No motion detected
  {
    if (MOV2occup) // No motion, Not yet published
    {
      MOV2movstatus = "MOV2 empty"; Particle.publish(stat_ROOM, MOV2movstatus);
      MOV2occup = 0;
    }

    if (MOV2light && (millis()-MOV2LightONLastTime) > MOV2LightONtime) // No motion, lights are ON and light time is up
    {
      MOV2Lightsoff();
    }
  }

  if ((millis()-MOV2totalLastTime) > MOV2totaltime) // Are the 5 min totalling time over?
  {
    MOV2movement = MOV2counter; // MOV2movement is now published!
    MOV2counter = 0; // Reset counter!
    MOV2totalLastTime = millis(); // Reset 5 minute totalling period
  }
} // endif ROOM setting "MOV2"

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
      TSTATon = 1; // Tstat = ON
      tstatTEMPstatus = "Tstat heat demand"; // For roomsensor area
    }
    else
    {
      TSTATon = 0; // Tstat = OFF
      tstatTEMPstatus = "No Tstat heat demand"; // For roomsensor area
    }
    Particle.publish(stat_HEAT, tstatTEMPstatus);
    getTstatLastTime = millis(); // Reset the timer
  }
} // endif ROOM setting "TSTAT"


// *A7: OPTIONAL connector: NONE, DIMMER, ALERTRCV, ...
if (strcmp(OP5_A7, "ALERTRCV") == 0)
{
  // *A7 - Alert sensor (LDR) = "AlertLDRpin"
  if ((millis()-getAlertLastTime) > getAlertInterval) // => It's time to check the LDR signal
  {
    readAlert();

    if (STOREAlertLevel < STORElightOnLevel && STOREmovstatus != "Alert monitored STOREs open") // STORE open => STOREAlertLevel LOW and status changed => lights ON
    {
      STOREalert = 1; // STORE alert is ON
      STOREmovstatus = "Alert monitored STOREs open"; Particle.publish(stat_ROOM, STOREmovstatus);
      // Only turn STORE light ON if Alert is "on" (enabled). If problems with Alert beam, turn Alert "off" with function manual("Alertoff")...
      if (Alert == "on") // Alert barrier is enabled (= Default)
      {
        STORElightsON(); // Turn STORE lights ON!
      }
    }

    // If STORE light ON time is expired, temporarily disable Alert and turn STORE lights OFF. (STORE left open!)
    if ((millis()-STORElightOnTime) >= MaxSTORElightOnTime && STOREmovstatus != "Alert monitored STOREs closed" && STORElightstatus != "STORE lights OFF") // STOREs open and time is up! (To avoid repeating, do it only if lights are ON...)
    {
      STORElightsOFF();
      Alert = "off"; // Disable the Alert barrier
    }

    if (STOREAlertLevel >= STORElightOnLevel && STOREmovstatus != "Alert monitored STOREs closed") // STOREs closed => STOREAlertLevel HIGH and status changed => lights OFF
    {
      STOREalert = 0; // STORE alert is OFF
      STORElightsOFF(); // Turn Alert STORE lights OFF!
      STOREmovstatus = "Alert monitored STOREs closed"; Particle.publish(stat_ROOM, STOREmovstatus);
      Alert = "on"; // When the Alert reaches the LDR, make sure the Alertbarrier is enabled!
    }

    getAlertLastTime = millis(); // Reset timer
  }
} // endif ROOM setting "ALERTRCV"


// *D3 - T-BUS: Reports to HVAC controller if there is condens danger. (= Std function)
if ((millis()-getTemperaturesLastTime)>getTemperaturesInterval) // To avoid "nervous" frequent switching, set this interval high enough!
{
  getTemperatures(0); // Update all sensor variables of array 0 (DS18B20 type)
  // Actions/calculations with T-BUS output:
  if (ROOMTemp1 < Tout) // Report if room temperature is close to the condensation limit (Tout is a few degrees higher for safety!) Added a max limit for Tout of 16°C...
  {
    outTEMPstatus = "Tout heat demand (Humidity)";
    TdfALERT = 1;
  }
  else
  {
    outTEMPstatus = "No Tout heat demand (Humidity)";
    TdfALERT = 0;
  }
  Particle.publish(stat_HEAT, outTEMPstatus);
  getTemperaturesLastTime = millis(); // Reset the timer
}



// *D5 - RoomSense MOV1 (= Std function)
if (digitalRead(MOV1pin) == HIGH) // Motion detected
{
  if ((millis()-MOV1counterLastTime) > 1000)
  {
    MOV1counter = MOV1counter + 1;
    MOV1counterLastTime = millis(); // Reset 1 minute period
  }
  MOV1LightONLastTime = millis(); // At each PIR trigger: reset ROOMlights turn OFF interval

  if (!MOV1occup) // Not occupied
  {
    MOV1movstatus = "MOV1 in use";
    Particle.publish(stat_ROOM, MOV1movstatus);
    MOV1occup = 1;

    if (ItIsNight && !MOV1light)
    {
      MOV1Lightson();
    }
  }
}

else // No motion detected
{
  if (MOV1occup) // No motion, Not yet published
  {
    MOV1movstatus = "MOV1 empty";
    Particle.publish(stat_ROOM, MOV1movstatus);
    MOV1occup = 0;
  }

  if (MOV1light && ((millis()-MOV1LightONLastTime) > MOV1LightONtime)) // No motion, lights are ON and light time is up
  {
    MOV1Lightsoff();
  }
}


if ((millis()-MOV1totalLastTime) > MOV1totaltime) // Are the 5 min totalling time over?
{
  MOV1movement = MOV1counter; // MOV1movement is now published!
  MOV1counter = 0; // Reset counter!
  MOV1totalLastTime = millis(); // Reset 5 minute totalling period
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
    Particle.publish(stat_ALERT, "CONDENS DANGER!"); // This is picked up by IFTTT => Notification sent to my iPhone
    sprintf(str, "Tdiff: %2.1f",Tdiff); Particle.publish(stat_HEAT, str);
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
    ItIsNight = 0; // it is day!
    if (TIMEstatus != "It is DAYTIME")
    {
      BedTime = 0; // No need anymore to dim the lights in this room. (Set remotely)
      TIMEstatus = "It is DAYTIME";
      Particle.publish(stat_LIGHT, TIMEstatus);
      // As long as the lights turn ON after every restart, turn them OF when it's DAYTIME:
      MOV1Lightsoff();
      MOV2Lightsoff();
    }
  }

  else // If SUNLightlevel <= Nightlevel
  {
    ItIsNight = 1; // It is night! (BedTime status for dimmed lights can now be set in function manual()...)
    if (TIMEstatus != "It is NIGHT")
    {
      TIMEstatus = "It is NIGHT";
      Particle.publish(stat_LIGHT, TIMEstatus);
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



// START Specific ROOM settings 2 for "INKOM2" (for 16-key touchpad) ////////////////////////////////////////////////////////////


void FunctionKey1() // Cycle UP through 4 display modes
{
  Displaymode = Displaymode + 1;
  if (Displaymode == 5)
  {
    Displaymode = 0;
  }
  display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Mode: "+String(Displaymode)); display.display();
  Particle.publish("action","Key-1");
  Displayfreeze = 0; // Un-freeze the display
}

void FunctionKey2() // Cycle DOWN through 4 display modes
{
  Displaymode = Displaymode - 1;
  if (Displaymode == 0)
  {
    Displaymode = 5;
  }
  display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Mode: "+String(Displaymode)); display.display();
  Particle.publish("action","Key-2");
  Displayfreeze = 0; // Un-freeze the display
}

void FunctionKey3()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-3"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("3:DISP+"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Show next display on OLED2..."); display.display();
}

void FunctionKey4()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-4"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("4:DISP-"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Show previous display on OLED2..."); display.display();
}


void FunctionKey5() // Default: Turn MOV1 lights ON for the preset time
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  MOV1Lightson();
  MOV1LightONLastTime = millis(); // Reset timer

  Particle.publish("action","Key-5"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("5:MOV1 ON"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Turn MOV1 lights on, whatever the time of the day. They will turn off after the preset time."); display.display();
}

void FunctionKey6() // Default: Force MOV1 lights OFF
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  MOV1Lightsoff();

  Particle.publish("action","Key-6"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("6:MOV1 OFF"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Force MOV1 lights off! They will turn ON normally when it's night and when MOV1 is activated..."); display.display();
}

void FunctionKey7() // Default: Force MOV2 lights ON for the preset time
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  MOV2Lightson();
  MOV2LightONLastTime = millis(); // Reset timer

  Particle.publish("action","Key-7"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("7:MOV2 ON"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Turn MOV2 lights on, whatever the time of the day. They will turn off after the preset time."); display.display();
}

void FunctionKey8() // Default: Force MOV2 lights OFF
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  MOV2Lightsoff();

  Particle.publish("action","Key-8"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("8:MOV2 OFF"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Force MOV2 lights off! They will turn ON normally when it's night and when MOV2 is activated..."); display.display();
}

void FunctionKey9()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  SUNLightlevel = 1000; // Sunlight level is set to DAYtime level (Value will normally be set by a "broadcast message" from a "daylight controller.

  Particle.publish("action","Key-9"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("DAY!"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Sunlight level is set to DAYtime level (Value will normally be set by a broadcast message from a daylight controller"); display.display();
}

void FunctionKey10()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  SUNLightlevel = 0; // Sunlight level is set to NIGHTtime level (Value will normally be set by a "broadcast message" from a "daylight controller.

  Particle.publish("action","Key-10"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("NIGHT!"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Sunlight level is set to NIGHT level (Value will normally be set by a broadcast message from a daylight controller"); display.display();
}

void FunctionKey11()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-11"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-11"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey12()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-12"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-12"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey13()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-13"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-13"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey14()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-14"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-14"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey15()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-15"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-15"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}

void FunctionKey16()
{
  Displayfreeze = 1; FreezeLastTime = millis(); // Freeze the display
  Particle.publish("action","Key-16"); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("Key-16"); display.setTextSize(1); display.setCursor(0,20);
  display.println("Do not show fast particle messages..."); display.display();
}
// STOP Specific ROOM settings 2 (for 16-key touchpad) ///////////////////////////////////////////////////////////////////////////




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
  //CO2ppm = CO2ppm/100; // = Optionally: This can be scaled for graphing purpose (Max value = 20)
}






// *A7 - OP2 => Alert sensor (LDR) = "AlertLDRpin"
void readAlert() // Check if Alert hits LDR or not => STOREAlertLevel => Control STORE lights
{
  STOREAlertLevel = 0;
  for(int x = 0 ; x < 20 ; x++)
  {
    STOREAlertLevel = STOREAlertLevel + analogRead(A7); // Use A7 directly: You can not create a name for this pin as it is used for 2 functions
  }
  STOREAlertLevel = STOREAlertLevel/20; // Average
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
  //Temporary: Commented next 3 lines to test if necessary
  //byte i;
  //byte present = 0;
  //byte data[12];
  byte addr[8];
  int sensorCount = 0;

  Particle.publish("OneWire", "Looking for 1-wire addresses:", 60, PRIVATE);

  while(ds.search(addr) and sensorCount < 20)
  {
    sensorCount++;
    char newAddress[48] = ""; // Make space for the 48 characters of our sensor addresses.
    snprintf(newAddress, sizeof(newAddress), "0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
    Particle.publish("OneWire", newAddress, 60, PRIVATE);

    if ( OneWire::crc8( addr, 7) != addr[7])
    {
      Particle.publish(stat_ALERT, "CRC is not valid!", 60, PRIVATE);
      return;
    }
  }

  ds.reset_search();
  Particle.publish("OneWire", "No more addresses!", 60, PRIVATE);
  return;
}




// *D4 - PIXEL-line - PARTICLE FUNCTION

/* "ledrgb" function: Runs when a color is picked in the corresponding "INKOM_RGB.html" webform (with this Photon's ID nr!)
You can also use the "rgb" function manually in the console or Particle App: Send a string of the format "0_255_255" for Magenta...
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

  // STORE the picker RGB values in the array:
  rgb[0] = redgreenblue[0].toInt();
  rgb[1] = redgreenblue[1].toInt();
  rgb[2] = redgreenblue[2].toInt();

  // Publish the selected colour:
  sprintf(str, "Colour:%d-%d-%d",rgb[0],rgb[1],rgb[2]);
  Particle.publish(stat_LIGHT, str);

  // Turn on all related RGB lights to confirm the set color:
  MOV1Lightson();
  MOV2Lightson();

  return 1; // Feedback when successful...
}


// START Specific ROOM settings 3 (for LIGHTING): "Room-INKOM"////////////////////////////////////////////////////////////

// *D4 - PIXEL-Line: (Attention: First in string = 0!)
// POWERPIXEL CONTROL:
void MOV1Lightson() // Turn lights ON.
{
  if (!BedTime)
  {
    // Normal lights command
    // Powerpixel 0 = Spanlampen inkom: Select an option...
    //strip.setPixelColor(0, strip.Color(255,255,255)); strip.show(); // Simple switching option: R= Spanlampen beneden, B= Spots onder palier (SIMPLE SWITCH ON = For testing)
    strip.setPixelColor(0, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb()
    //DimUpGroups(0, 5); // Dimming option: Check "flicker" issues! PowerPiXel 0 = SpanBeneden, Min = 0, Steps = 10 (slow)

    // Powerpixel 2 = Staircase RGB strips: Select an option...
    //strip.setPixelColor(2, strip.Color(255,255,255)); strip.show(); // Simple switching option (For testing)
    //DimUpGroupsRG(2, 0, 5); // PowerPiXel 2 = RGB, Min = 0, Steps = 10 (slow)
    strip.setPixelColor(2, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb() above!
  }
  else
  {
    // Bedtime! Dimmed lights: Only staircase strip! Powerpixel 2
    strip.setPixelColor(2, strip.Color(0,0,100)); strip.show(); // Light BLUE Simple switching option (For testing)
  }


  MOV1lightstatus = "MOV1 light is ON"; Particle.publish(stat_LIGHT, MOV1lightstatus);
  MOV1light = 1;
}

void MOV1Lightsoff() // Turn lights OFF per colour: R= Spanlampen beneden, G= Spots onder palier
{
  // Simple switching option:
  strip.setPixelColor(0, strip.Color(0,0,0)); // SIMPLE SWITCH OFF = For testing
  strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // RGB lights (SIMPLE SWITCH OFF = For testing)

  // Dimming option: Check "flicker" issues!
  //DimDownGroups(2, 5); // PowerPiXel 2 = RGB, Steps = 10 (slow)
  //DimDownGroups(0, 5); // PowerPiXel 0 = SpanBeneden, Steps = 10 (slow)

  MOV1lightstatus = "MOV1 light is OFF"; Particle.publish(stat_LIGHT, MOV1lightstatus);
  MOV1light = 0;
}


void MOV1demo() // One cycle dimming ON/OFF...
{
  MOV1Lightson();
  delay(5000);
  MOV1Lightsoff();
}


// MOV2 lights CONTROL. 2 variants: 1) If A7 = MOV2 dimmer pin => Send PWM signal from A7; 2) Else: Control POWERPIXELs.
void MOV2Lightson() // Turn second group of lights ON
{
  if (!BedTime)
  {
    // Normal night lights command:
    if (strcmp(OP5_A7, "DIMMER") == 0) // Variant 1: With PWM dimmer
    {
      // Put here the A7 dimmer commands (BandB)
      // Use MOV2DIMMERpin for PWM output
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

  }
  else
  {
    // Bedtime! Dimmed lights command:
    if (strcmp(OP5_A7, "DIMMER") == 0) // Variant 1: With PWM dimmer
    {
      // Put here the A7 dimmer commands (BandB)
      // Use MOV2DIMMERpin for PWM output
      while (DimLightLevel > 0)
      {
        DimLightLevel--;
        analogWrite(A7, DimLightLevel); // Use A7 directly; You cannot create a name for this pin as it may be used for different functions
        delay(DimStep);
      }
    } // endif ROOM setting "DIMMER"

    else // Variant 2: With Powerpixels
    {
      // Bedtime! No lights anymore here! Powerpixel 1 => Add LEDstrips on wall later...
      strip.setPixelColor(1, strip.Color(0,0,0)); strip.show(); // Light OFF Simple switching option (For testing)
    } // endif ROOM setting not "DIMMER"
  }

  // Whatever output is used, report MOV2lightstatus:
  MOV2lightstatus = "MOV2 light is ON"; Particle.publish(stat_LIGHT, MOV2lightstatus);
  MOV2light = 1;
}

void MOV2Lightsoff() // Turn second group of lights OFF
{
  if (strcmp(OP5_A7, "DIMMER") == 0) // Variant 1: With PWM dimmer
  {
    // Put here the A7 dimmer commands (BandB)
    // Use MOV2DIMMERpin for PWM output
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

  // Whatever output is used, report MOV2lightstatus:
  MOV2lightstatus = "MOV2 light is OFF"; Particle.publish(stat_LIGHT, MOV2lightstatus);
  MOV2light = 0;
}

void MOV2demo() // One cycle dimming ON/OFF...
{
  MOV2Lightson();
  delay(5000);
  MOV2Lightsoff();
}


// STORE lights: Responding to the ALERTRCV signal (A7)
void STORElightsON() // Turn STORE lights ON
{
  STORElight = 1;
  STORElightOnTime = millis(); // Restart the STORE light ON timer. Will turn OFF lights after expiring...
  strip.setPixelColor(3, strip.Color(255,255,255)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power ON
  STORElightstatus = "STORE lights ON"; Particle.publish(stat_LIGHT, STORElightstatus);
}

void STORElightsOFF() // Turn STORE lights (Powerpixel 3) OFF
{
  STORElight = 0;
  strip.setPixelColor(3, strip.Color(0,0,0)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power OFF
  STORElightstatus = "STORE lights OFF"; Particle.publish(stat_LIGHT, STORElightstatus);
}
// STOP Specific ROOM settings 2 (for LIGHTING): "Room-INKOM"////////////////////////////////////////////////////////////




// BASIC POWERPIXEL CONTROL FUNCTIONS:
// 1. Increases brightness of all groups of a PowerPixel in steps. Parameters: Pixel #, Start - End, Nr of steps (1 = fast, more = slower) ex: DimUp(0, 0, 255, 5);
void DimUp(uint8_t Nr, uint8_t min, uint8_t max, uint8_t wait)
{
  for(int i=min;i<max;i++)
  {
    strip.setPixelColor(Nr, strip.Color(i,i,i));
    strip.show();
    delay(wait);
  }
}

// 2. Decreases brightness of all groups of a PowerPixel in steps: Parameters: Pixel #, Start - End, Nr of steps (1 = fast, more = slower) ex: DimDown(0, 255, 0, 5);
void DimDown(uint8_t Nr, uint8_t max, uint8_t min, uint8_t wait)
{
  for(int i=max;i>=min;i--)
  {
    strip.setPixelColor(Nr, strip.Color(i,i,i));
    strip.show();
    delay(wait);
  }
}

// 3. Fades each group of a PowerPixel OFF in turn: Parameters: Pixel #, Minimum, Nr of steps (1 = fast, more = slower) ex: DimDownGroups(0, 5);
void DimDownGroups(uint8_t Nr, uint8_t wait)
{
  for(int i=255;i>=0;i--)
  {
    strip.setPixelColor(Nr, strip.Color(i,255,255));
    strip.show();
    delay(wait);
  }
  for(int i=255;i>=0;i--)
  {
    strip.setPixelColor(Nr, strip.Color(0,i,255));
    strip.show();
    delay(wait);
  }
  for(int i=255;i>=0;i--)
  {
    strip.setPixelColor(Nr, strip.Color(0,0,i));
    strip.show();
    delay(wait);
  }
}

// 4. Fades each group of a PowerPixel ON in turn: Parameters: Pixel #, Minimum, Nr of steps (1 = fast, more = slower) ex: DimUpGroups(0, 5);
void DimUpGroups(uint8_t Nr, uint8_t wait)
{
  for(int i=0;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(i,0,0));
    strip.show();
    delay(wait);
  }

  for(int i=0;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(255,i,0));
    strip.show();
    delay(wait);
  }
  for(int i=0;i<255;i++)
  {
    strip.setPixelColor(Nr, strip.Color(255,255,i));
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

  // As long as the OneWire sensor publishes 0, make Temp1 = Temp2
  /* SWITCH OFF when not necessary
  if (ROOMTemp1 == 0)
  {
    ROOMTemp1 = ROOMTemp2;
  }
  */
}



// COMMON Particle Function(s):

// A. Receive the device NAME from Particle cloud
void DevNamereceiver(const char *topic, const char *name)
{
  strncpy(device_name, name, sizeof(device_name)-1); // STORE the device name in a string

  snprintf(stat_ROOM, sizeof(str), "Status-ROOM:%s", (const char*)device_name); // Create the Status-ROOM event name with the device name at the end
  snprintf(stat_LIGHT, sizeof(str), "Status-LIGHT:%s", (const char*)device_name); // Create the Status-LIGHT event name with the device name at the end
  snprintf(stat_HEAT, sizeof(str), "Status-HEAT:%s", (const char*)device_name); // Create the Status-HEAT event name with the device name at the end
  snprintf(stat_ALERT, sizeof(str), "Status-Alert:%s", (const char*)device_name); // Create the Status-Alert event name with the device name at the end
}

// B. *OLED display: Receiving and displaying the Particle events
void EventDecoder(const char *event, const char *data) // = Called when an event starting with "Status-" prefix is published...
{
  // 1. Create the Name and Subject variables from the messages received:
  char* Name = strtok(strdup(event), ""); // = Type of message
  char* Subject = strtok(strdup(data), ""); // = Message itself
  // Explanation of command "strtok(strdup(event), "")": => Take first string until delimiter = ":" (If there is no : then the full string is copied!)

  if (strcmp(I2C_D0D1, "OLED") == 0 && Displaymode == 0 && !Displayfreeze) // Only show the messages if Displaymode = 0 and display is not frozen
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
  if(command == "reportsetting")
  {
    // Room_settings
    Particle.publish("Setting I2C_D0D1", I2C_D0D1);
    Particle.publish("Setting OP5_A7", OP5_A7);
    Particle.publish("Setting OP4_A6", OP4_A6);
    Particle.publish("Setting OP3_A5", OP3_A5);
    Particle.publish("Setting OP2_A4", OP2_A4);
    Particle.publish("Setting GAS_A2D7", GAS_A2D7);
    Particle.publish("Setting OP1_A1", OP1_A1);
    Particle.publish("Setting INT_A0", INT_A0);

    return 1001;
  }


  if(command == "reportheat")
  {
    // ROOMSENSE box std functions
    sprintf(str, "Temp1:%2.1f",ROOMTemp1);
    Particle.publish(stat_HEAT, str);
    sprintf(str, "Temp2:%2.1f Humid:%2.0f Dew:%2.1f Tout:%2.1f",ROOMTemp2,ROOMHumi,ROOMTdf,ROOMTout);
    if (ROOMTemp1 == ROOMTemp2)
    {
      Particle.publish(stat_HEAT, "Waiting OneWire: ROOMTemp1 = ROOMTemp2!");
    }
    Particle.publish(stat_HEAT, str);
    Particle.publish(stat_HEAT, outTEMPstatus);
    Particle.publish(stat_HEAT, tstatTEMPstatus);

    return 1002;
  }


  if(command == "reportlight")
  {
    sprintf(str, "SUNLIGHT:%2.0f",SUNLightlevel);
    Particle.publish(stat_LIGHT, str);
    Particle.publish(stat_LIGHT, TIMEstatus);

    // ROOMSENSE box std functions
    sprintf(str, "Room Light:%2.0f",ROOMLightlevel);
    Particle.publish(stat_LIGHT, str);

    // All RGB related lights: Selected RGB colour
    sprintf(str, "Colour:%d-%d-%d",rgb[0],rgb[1],rgb[2]);
    Particle.publish(stat_LIGHT, str);
    // MOV1 Lights (= STD)
    Particle.publish(stat_LIGHT, MOV1lightstatus);

    // MOV2 Lights (OPTional)
    if (strcmp(OP3_A5, "MOV2") == 0)
    {
      Particle.publish(stat_LIGHT, MOV2lightstatus);
    } // endif ROOM setting "MOV2"

    // STORE Alert barrier & Lights
    if (strcmp(OP5_A7, "ALERTRCV") == 0)
    {
      Particle.publish(stat_LIGHT, STORElightstatus);
    } // endif ROOM setting "ALERTRCV"

    return 1003;
  }


  if(command == "reportroom")
  {
    // GENERAL
    // Date/time stamp
    Particle.publish(stat_ROOM, Time.timeStr()); // eg: Wed May 21 01:08:47 2014
    Particle.publish(stat_ROOM, TIMEstatus);

    // System health data (JSON) => This is updated together with the homebridge data!
    Particle.publish(stat_ROOM, JSON_status);

    // ROOMSENSE box optional CO2
    if (strcmp(OP2_A4, "CO2_PWM") == 0)
    {
      sprintf(str, "CO2 x100:%2.0f",CO2ppm);
      Particle.publish(stat_ROOM, str);
    } // endif ROOM setting "CO2_PWM"

    // ROOMSENSE box optional DUST
    if (strcmp(GAS_A2D7, "DUST") == 0)
    {
      sprintf(str, "DUST Pct:%2.0f",Dust);
      Particle.publish(stat_ROOM, str);
    } // endif ROOM setting "DUST"

    // MOV1 Lights (= STD)
    sprintf(str, "MOV1movement:%2.0f",MOV1movement);
    Particle.publish(stat_ROOM, str);
    Particle.publish(stat_ROOM, MOV1movstatus);

    // MOV2 Lights (OPTional)
    if (strcmp(OP3_A5, "MOV2") == 0)
    {
      sprintf(str, "MOV2movement:%2.0f",MOV2movement);
      Particle.publish(stat_ROOM, str);
      Particle.publish(stat_ROOM, MOV2movstatus);
    } // endif ROOM setting "MOV2"

    // STORE Alert barrier & Lights
    if (strcmp(OP5_A7, "ALERTRCV") == 0)
    {
      sprintf(str, "STOREAlert:%2.0f",STOREAlertLevel);
      Particle.publish(stat_ROOM, str);
      Particle.publish(stat_ROOM, STOREmovstatus);
    } // endif ROOM setting "ALERTRCV"

    return 1004;
  }


  if(command == "reportsensors") // Currently only 1-wire sensors
  {
    discoverOneWireDevices();
    return 1005;
  }


  if((command == "mov1on") || (command == "Lights1=1}")) // Second condition is for Homebridge
  {
    MOV1Lightson();
    MOV1LightONLastTime = millis(); // Reset turn OFF interval
    return 100;
  }


  if((command == "mov1off") || (command == "Lights1=0}")) // Second condition is for Homebridge
  {
    MOV1Lightsoff();
    return 1;
  }


  if(command == "mov1demo") //
  {
    MOV1demo();
    return 1;
  }


  if (strcmp(OP3_A5, "MOV2") == 0)
  {
    if((command == "mov2on") || (command == "Lights2=1}")) // Second condition is for Homebridge
    {
      MOV2Lightson();
      MOV2LightONLastTime = millis(); // Reset turn OFF interval
      return 200;
    }


    if((command == "mov2off") || (command == "Lights2=0}")) // Second condition is for Homebridge
    {
      MOV2Lightsoff();
      return 2;
    }


    if(command == "mov2demo") //
    {
      MOV2demo();
      return 1;
    }
  } // endif ROOM setting "MOV2"


  if (strcmp(OP5_A7, "ALERTRCV") == 0) // Only if a Alertbeam is installed
  {

    if(command == "alerton") // Enable the STORE Alert barrier and let CAB lights automatically react to it
    {
      Alert = "on";
      return 1;
    }


    if(command == "alertoff") // Disable the STORE Alert barrier and turn CAB lights OFF
    {
      Alert = "off";
      STORElightsOFF();
      return 0;
    }


    if(command == "alertlighton")
    {
      STORElightsON();
      return 300;
    }


    if(command == "alertlightoff")
    {
      STORElightsOFF();
      return 3;
    }
  } // endif ROOM setting "ALERTRCV"


  if(command == "night") // Manual mode change if no sunlight info is received...
  {
    SUNLightlevel = 0; // Sunlight level is set to NIGHTtime level (Value will normally be set by a "broadcast message" from a "daylight controller.
    return 100;
  }


  if(command == "day") // Manual mode change if no sunlight info is received...
  {
    SUNLightlevel = 1000; // Sunlight level is set to DAYtime level (Value will normally be set by a "broadcast message" from a "daylight controller.
    return 1000;
  }


  if(command == "bedtime") // Manual mode change if no sunlight info is received...
  {
    BedTime = 1; // BedTime status: dim the lights in this room!
    return 0;
  }


  if(command == "wakeup")
  {
    BedTime = 0; // No BedTime status: stop dimming the lights in this room!
    return 100;
  }


  if(command == "reset") // You can remotely RESET the photon with this command... (It won't reset the IO-eXbox!)
  {
    System.reset();
    return -10000;
  }


  Particle.publish(stat_ROOM, command);  // If it does not match one of the above, publish the received string to see what it's "payload" was...
  return -1;// If none above
}
