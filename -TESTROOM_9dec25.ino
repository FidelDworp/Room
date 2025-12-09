// -TESTROOM.ino = Test sketch for test setup (Based on R3-INKOM)
//
// 9dec25: Combined R3-INKOM + CO2 code from R1-BandB.
//
// ---------------------------------------
// PhotoniX shield v.3.0	I/O connections: (* = used)
//
//  D0/D1 - COM1/2 (I2C-SDA & SCL) = OLED display, I/O expander, Sunlight sensor (TSL2561)...
//  D2 - COM3 = 16-key touchpad (CLK) or TOUCH-COMMON
// *D3 - RoomSense T-BUS
// *D4 - OP5 = PIXEL-line
// *D5 - RoomSense MOV1
// *D6 - RoomSense TEMP/HUM
// *D7 - RoomSense GAS-DIG or DUST sensor (LED transmitter)
//  A0 - COM4 = 16-key touchpad (DATA1) = TOUCH-1
//  A1 - COM5 = DOTSTAR-line (Data) or 16-key touchpad (DATA2) = TOUCH-2
// *A2 - RoomSense GAS-ANA or DUST sensor (LED receiver)
// *A3 - RoomSense LIGHT => Put 22k series resistor with LDR (Not 5V tolerant!)
// *A4 - OP3 = PWM input for CO2
// *A5 - OP4 = MOV2 (Second light group), LCD-Reset or DOTSTAR-line (Clock)
// *A6 - OP1 (Not 5V!) = Thermostat switch to GND
// *A7 - OP2 = MOV2 operated dimmer pin or Alert light sensor (LDR) input.
//  TX/RX - COM6/7 = Serial comms
//  ---------------------------------------

// *D3: ROOMSENSE connector:
byte addrs0[1][8] = {{0x28,0xFF,0x1A,0x3A,0x33,0x17,0x04,0x03}};  // TESTROOM: New Round RoomSenseBoX

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

// Strings for publishing
char str[255]; // Temporary string for all messages
char JSON_status[400]; // Publish all status variables for Ken's App in one string (Max 622)

// For storing Particle device NAME in event name groups:
char device_name[32] = "";
char stat_ROOM[40];
char stat_LIGHT[40];
char stat_HEAT[40];
char stat_ALERT[40];

// Homebridge reporting (unused, but keep these for reporting in general)
int HomebridgeInterval = 120 * 1000;
int HomebridgeLastTime = millis() - HomebridgeInterval;

// Sunlight level (Broadcasted by another Photon)
double SUNLightlevel = 0; // Sunlight level from the (external) SOLAR sensor => Initially set to NIGHT- or DAYtime level (Value will be set from a "broadcast message" from a "daylight controller.
int Nightlevel = 200; // Below this SUNLightlevel it is NIGHT! (broadcasted by another Photon running "I2C-Daylightsensor.ino")
boolean ItIsNight = 0; // This will be set to 1 or 0, depending on "checkDAY_NIGHT()" function. (Initialized as DAY)
String TIMEstatus = "Initialized as DAY!"; // Temporary string
boolean BedTime = 0;// BedTime status: Remotely created in function manual()

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
int MOV2totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter
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

// *A7 STORE Alert (ex: laser barrier) sensor (LDR)
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

// *D5 - RoomSense MOV1:
int MOV1pin = D5;
int MOV1counter = 0; // Counts MOV1 triggers every second
int MOV1counterLastTime;
double MOV1movement = 0; // Total movement over last 5 minutes
int MOV1totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter
int MOV1totalLastTime;
String MOV1movstatus = "MOV1 empty"; // Initial setting
boolean MOV1occup = 0; // Is this occupied?
String MOV1lightstatus = "MOV1 light is OFF"; // Initial setting
boolean MOV1light = 0; // Is this light ON?
int MOV1LightONtime = 5 * 60 * 1000; // How long should the lights stay ON? (5 min)
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
  Particle.variable("JSON_status", JSON_status, STRING);

  // Initialize Particle subscribe function:
  // Catching private events
  Particle.subscribe("Status-", EventDecoder, MY_DEVICES); // Listening for the event "Status-*" and, when it receives it, it will run the function EventDecoder()
  // Catching it's own device NAME
  Particle.subscribe("particle/device/name", DevNamereceiver); // Listening for the device name...
  delay(1000); // Give it some time...

  // *A2 - RoomSense GAS-ANA +  D7 - RoomSense GAS-DIG
  pinMode(LEDPin,OUTPUT);
  pinMode(DUSTPin,INPUT_PULLUP); // Dust sensor. Input forced HIGH. <analogread> automatically switches pullup OFF (https://docs.particle.io/reference/firmware/photon/#pinmode-)
  Particle.variable("DUSTlevel", &Dust, DOUBLE);

  // *A3 - RoomSense LIGHT
  pinMode(LIGHTpin,INPUT_PULLDOWN); // Light sensor. Input forced LOW. (If no sensor connected: 0)
  Particle.variable("ROOM_Light", &ROOMLightlevel, DOUBLE);

  // *A4 - OP3 => CO2 PWM
  Particle.variable("CO2ppm", &CO2ppm, DOUBLE);

  // *A5 - MOV2:
  pinMode(MOV2pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
  Particle.variable("MOV2movement", &MOV2movement, DOUBLE);

  // *A6 - OP1 (Not 5V!) => Thermostat
  pinMode(TSTATpin, INPUT_PULLUP); // Thermostat. Input forced HIGH.

  // *A7 = Alert light sensor (LDR) = "AlertLDRpin" (for STORE lights)
  int AlertLDRpin = A7;
  pinMode(AlertLDRpin, INPUT); // Alert sensor (LDR) input
  // TEMPORARY for testing purpose: Check Alertbeam operation!
  // Particle.variable("AlertLevel", &STOREAlertLevel, DOUBLE);


  // *D3 - RoomSense T-BUS
  for (int i = 0; i < sizeof(temps)/sizeof(temps[0]); i++)
  {
    tmStamp[i] = Time.now();
  }
  //Particle.variable("CRC_Errors", crcErrorJSON, STRING); // Only needed for debugging: Shows accumulated number of errors...
  Particle.variable("ROOM_Temp1", &ROOMTemp1, DOUBLE);

  // START Specific ROOM settings 2 for accessories: "Room-INKOM"////////////////////////////////////////////////////////////

  // *D4 - PIXEL-line
  Particle.function("rgb", ledrgb); // Show currently selected colour value from webpage (For debugging only!)
  strip.begin(); strip.show(); // Initialize all pixels to 'off'

  // Initialize RGB color, until "mobile color picker" is used (Maarten's choice!);
  // ATTENTION: This MUST be in setup() !!!
  rgb[0] = 219; // Red      (0-255)
  rgb[1] = 159; // Green    (0-255)
  rgb[2] = 61;  // Blue     (0-255)

  // STOP Specific ROOM settings 2 for RGB settings: "Room-INKOM"////////////////////////////////////////////////////////////

  // *D5 - RoomSense MOV1 (Default)
  pinMode(MOV1pin, INPUT_PULLUP); // IMPORTANT!!! Input forced HIGH.
  Particle.variable("MOV1movement", &MOV1movement, DOUBLE);

  // *D6 - RoomSense TEMP/HUM
  Particle.variable("ROOM_Temp2", ROOMTemp2);
  Particle.variable("ROOM_Humid",ROOMHumi);
  Particle.variable("ROOM_Tout", ROOMTout);

  DHT.begin();
} // end setup






void loop()
{
  // 27sep21: https://community.particle.io/t/surprising-issue-after-flashing-new-sketch-to-my-10-roomcontrollers/61194/18
  if (!strlen(device_name)) // If the variable has received contents...
  Particle.publish("particle/device/name"); // Ask the cloud (once) to send the device NAME!

  // General commands:
  // 1. Check memory
  freemem = System.freeMemory();// For debugging: Track if memory leak exists...
  memPERCENT = (freemem/51944)*100;

  if(memPERCENT < 30)
  {
    Particle.publish(stat_ALERT, "MEMORY LEAK in controller: Restarting!",60,PRIVATE);
    System.reset();
  }

  // 2. Check if it is day or night:
  checkDAY_NIGHT();

  // 3. REPORT Std Roomsense data
  // 1) To Homebridge:
  if ((millis()-HomebridgeLastTime) > HomebridgeInterval)
  {
    HomebridgeLastTime = millis(); // Reset reporting timer

    // NEW 10may20: Store the colour array values in simple variables:
    int Rval = rgb[0];
    int Gval = rgb[1];
    int Bval = rgb[2];

    // NEW 10may20: Ken's JSON string, generated by Generate.html with single character names:
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
}


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


// *A3 - RoomSense LIGHT = Std function!
if ((millis()-getLightLastTime) > getLightInterval)
{
  ReadLight();
  // Put eventual extra actions/calculations with LIGHT here...
  getLightLastTime = millis();
}

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
  Particle.publish(stat_ROOM, "Room CO2 ppm HIGH",60,PRIVATE);
}

if (CO2ppm > 1800) // CO2 level too high!
{
  Particle.publish(stat_ROOM, "Room CO2 ppm too HIGH",60,PRIVATE);
}

getCO2LastTime = millis(); // Reset timer
} // endif time to update!


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
    MOV2movstatus = "MOV2 in use";
    //Particle.publish(stat_ROOM, MOV2movstatus, 60,PRIVATE);
    MOV2occup = 1;

    if (ItIsNight && !MOV2light)
    {
      MOV2Lightson();
      MOV2LightONLastTime = millis(); // Reset turn OFF interval (6sep22: Added this after comparing w MOV1!)
    }
  }
}

else // No motion detected
{
  if (MOV2occup) // No motion, Not yet published
  {
    MOV2movstatus = "MOV2 empty";
    //Particle.publish(stat_ROOM, MOV2movstatus, 60,PRIVATE);
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
  Particle.publish(stat_HEAT, tstatTEMPstatus, 60,PRIVATE);
  getTstatLastTime = millis(); // Reset the timer
}


// *A7 - Alert sensor (LDR) = "AlertLDRpin"
if ((millis()-getAlertLastTime) > getAlertInterval) // => It's time to check the LDR signal
{
  readAlert();

  if (STOREAlertLevel < STORElightOnLevel && STOREmovstatus != "Alert monitored STOREs open") // STORE open => STOREAlertLevel LOW and status changed => lights ON
  {
    STOREalert = 1; // STORE alert is ON
    STOREmovstatus = "Alert monitored STOREs open"; Particle.publish(stat_ROOM, STOREmovstatus, 60,PRIVATE);
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
    STOREmovstatus = "Alert monitored STOREs closed"; Particle.publish(stat_ROOM, STOREmovstatus, 60,PRIVATE);
    Alert = "on"; // When the Alert reaches the LDR, make sure the Alertbarrier is enabled!
  }

  getAlertLastTime = millis(); // Reset timer
}


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
  Particle.publish(stat_HEAT, outTEMPstatus, 60,PRIVATE);
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
    //Particle.publish(stat_ROOM, MOV1movstatus, 60,PRIVATE);
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
    //Particle.publish(stat_ROOM, MOV1movstatus, 60,PRIVATE);
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
    Particle.publish(stat_ALERT, "CONDENS DANGER!", 60,PRIVATE); // This is picked up by IFTTT => Notification sent to my iPhone
    sprintf(str, "Tdiff: %2.1f",Tdiff); Particle.publish(stat_HEAT, str,60,PRIVATE);
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
      Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE);
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
      Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE);
    }
  }
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

/* "ledrgb" function: Runs when a color is picked in the corresponding "INKOM_RGB.html" webform (with this Photon's ID nr!)
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
  Particle.publish(stat_LIGHT, str,60,PRIVATE);

  // Turn on all related RGB lights to confirm the set color:
  MOV1Lightson();
  MOV2Lightson();

  return 1; // Feedback when successful...
}


// START Specific ROOM settings 3 for LIGHTING: "Room-INKOM"////////////////////////////////////////////////////////////

// *D4 - PIXEL-Line: (Attention: First in string = 0!)
// ATTENTION: While studying the "flicker" issues, choose "dimming" or "simple switching" settings below!

// INKOM POWERPIXEL CONTROL:
void MOV1Lightson() // Turn lights ON.
{
  // Powerpixel 2 = Both Trap & Palier ON (Inkom spanlampen uit)  (Attention: For 2 strip commands, only use one strip.show command!)
  strip.setPixelColor(2, strip.Color(120,120,0)); // Trap light RG ON = Yellow
  strip.setPixelColor(0, strip.Color(0,0,255)); strip.show(); // Palier ON (op Blue poort)
  //strip.setPixelColor(2, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb() above!

  MOV1lightstatus = "MOV1 light is ON";
  //Particle.publish(stat_LIGHT, MOV1lightstatus, 60,PRIVATE); delay(500);
  MOV1light = 1;
}

void MOV1Lightsoff() // Turn lights OFF: R= Spanlampen beneden, G= Spots onder palier
{
  // Powerpixel 2 = Both Trap & Inkom OFF (Attention: For 2 strip commands, only use one strip.show command!)
  strip.setPixelColor(2, strip.Color(0,0,0)); // Trap RGB OFF
  strip.setPixelColor(0, strip.Color(0,0,0)); strip.show(); // Inkom + Palier OFF

  MOV1lightstatus = "MOV1 light is OFF";
  //Particle.publish(stat_LIGHT, MOV1lightstatus, 60,PRIVATE); delay(500);
  MOV1light = 0;
}


void MOV1demo() // One cycle dimming ON/OFF...
{
  MOV1Lightson();
  delay(5000);
  MOV1Lightsoff();
}


void MOV2Lightson() // Turn second group of lights ON
{
  // Simple switching option:
  //strip.setPixelColor(2, strip.Color(0,0,10)); strip.show(); // RGB lights (SIMPLE SWITCH OFF = For testing)
  //strip.setPixelColor(2, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb() above!
  // Report MOV2lightstatus:
  MOV2lightstatus = "MOV2 light is ON";
  //Particle.publish(stat_LIGHT, MOV2lightstatus, 60,PRIVATE); delay(500);
  MOV2light = 1;
}

void MOV2Lightsoff() // Turn second group of lights OFF
{
  // Simple switching option:
  //strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // RGB lights (SIMPLE SWITCH OFF = For testing)
  // Report MOV2lightstatus:
  MOV2lightstatus = "MOV2 light is OFF";
  //Particle.publish(stat_LIGHT, MOV2lightstatus, 60,PRIVATE); delay(500);
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
  STORElightstatus = "STORE lights ON";
  //Particle.publish(stat_LIGHT, STORElightstatus, 60,PRIVATE); delay(500);
}

void STORElightsOFF() // Turn STORE lights (Powerpixel 3) OFF
{
  STORElight = 0;
  strip.setPixelColor(3, strip.Color(0,0,0)); strip.show(); // PowerPiXel 3 (= fourth) => SIMPLE SWITCH ON = SSR switches 230V led power OFF
  STORElightstatus = "STORE lights OFF";
  //Particle.publish(stat_LIGHT, STORElightstatus, 60,PRIVATE); delay(500);
}
// STOP Specific ROOM settings 3 for LIGHTING: "Room-INKOM"////////////////////////////////////////////////////////////




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

// 3. Fades each group of a PowerPixel OFF in turn: Parameters: Pixel #, Nr of steps (1 = fast, more = slower) ex: DimDownGroups(0, 5);
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

// 4. Fades each group of a PowerPixel ON in turn: Parameters: Pixel #, Nr of steps (1 = fast, more = slower) ex: DimUpGroups(0, 5);
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

  // As long as the OneWire sensor did not publish a realistic value, make Temp1 = Temp2
  if (ROOMTemp1 == 0)
  {
    ROOMTemp1 = ROOMTemp2;
  }
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

  // OTHER LIGHTS:
  if((command == "inkomon") || (command == "Inkom=1}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(0, strip.Color(255,255,255)); strip.show(); // Simple switch ON
    MOV1light = 1;
    return 100;
  }

  if((command == "inkomoff") || (command == "Inkom=0}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(0, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    MOV1light = 0;
    return 0;
  }

  if((command == "dressingon") || (command == "Dressing=1}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(1, strip.Color(255,255,255)); strip.show(); // Simple switch ON
    MOV2light = 1;
    return 200;
  }

  if((command == "dressingoff") || (command == "Dressing=0}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(1, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    MOV2light = 0;
    return 0;
  }

  if((command == "trapon") || (command == "Trap=1}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb() above!
    return 300;
  }

  if((command == "trapoff") || (command == "Trap=0}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    MOV1light = 0;
    return 0;
  }

  if((command == "traproodon") || (command == "Traprood=1}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(0,255,0)); strip.show(); // Simple switch RED on
    return 0;
  }

  if((command == "traproodoff") || (command == "Traprood=0}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    MOV1light = 0;
    return 0;
  }

  if((command == "trapgroenon") || (command == "Trapgroen=1}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(255,0,0)); strip.show(); // Simple switch GREEN on
    return 0;
  }

  if((command == "trapgroenoff") || (command == "Trapgroen=0}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    MOV1light = 0;
    return 0;
  }

  if((command == "trapblauwon") || (command == "Trapblauw=1}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(0,0,255)); strip.show(); // Simple switch BLUE on
    return 0;
  }

  if((command == "trapblauwoff") || (command == "Trapblauw=0}")) // Second condition is for Homebridge
  {
    strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    MOV1light = 0;
    return 0;
  }

  // PIXELTEST: Test individual channels of all connected powerpixels
    // Pixel 0
    if(command == "0r") // Pixel 0, channel R
    {
      strip.setPixelColor(0, strip.Color(255,0,0)); strip.show();
      return 99;
    }
    if(command == "0g") // Pixel 0, channel G
    {
      strip.setPixelColor(0, strip.Color(0,255,0)); strip.show();
      return 99;
    }
    if(command == "0b") // Pixel 0, channel B
    {
      strip.setPixelColor(0, strip.Color(0,0,255)); strip.show();
      return 99;
    }
    if(command == "0off") // Pixel 0, OFF
    {
      strip.setPixelColor(0, strip.Color(0,0,0)); strip.show();
      return 0;
    }
    // Pixel 1
    if(command == "1r") // Pixel 1, channel R
    {
      strip.setPixelColor(1, strip.Color(255,0,0)); strip.show();
      return 99;
    }
    if(command == "1g") // Pixel 1, channel G
    {
      strip.setPixelColor(1, strip.Color(0,255,0)); strip.show();
      return 99;
    }
    if(command == "1b") // Pixel 1, channel B
    {
      strip.setPixelColor(1, strip.Color(0,0,255)); strip.show();
      return 99;
    }
    if(command == "1off") // Pixel 1, OFF
    {
      strip.setPixelColor(1, strip.Color(0,0,0)); strip.show();
      return 0;
    }
    // Pixel 2
    if(command == "2r") // Pixel 2, channel R
    {
      strip.setPixelColor(2, strip.Color(255,0,0)); strip.show();
      return 99;
    }
    if(command == "2g") // Pixel 2, channel G
    {
      strip.setPixelColor(2, strip.Color(0,255,0)); strip.show();
      return 99;
    }
    if(command == "2b") // Pixel 2, channel B
    {
      strip.setPixelColor(2, strip.Color(0,0,255)); strip.show();
      return 99;
    }
    if(command == "2off") // Pixel 2, OFF
    {
      strip.setPixelColor(2, strip.Color(0,0,0)); strip.show();
      return 0;
    }
    // Pixel 3
    if(command == "3r") // Pixel 1, channel R
    {
      strip.setPixelColor(3, strip.Color(255,0,0)); strip.show();
      return 99;
    }
    if(command == "3g") // Pixel 1, channel G
    {
      strip.setPixelColor(3, strip.Color(0,255,0)); strip.show();
      return 99;
    }
    if(command == "3b") // Pixel 1, channel B
    {
      strip.setPixelColor(3, strip.Color(0,0,255)); strip.show();
      return 99;
    }
    if(command == "3off") // Pixel 3, OFF
    {
      strip.setPixelColor(3, strip.Color(0,0,0)); strip.show();
      return 0;
    }
  // END OF PIXELTEST

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

  if(command == "reset") // You can remotely RESET the photon with this command... (It won't reset the IO-eXbox!)
  {
    System.reset();
    return -10000;
  }

  Particle.publish(stat_ROOM, command,60,PRIVATE);  // If it does not match one of the above, publish the received string to see what it's "payload" was...
  return -1;// If none above

}
