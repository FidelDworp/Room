// -ROOM_R6-EETPL_21oct23.ino = similar to KEUK sketch - Installed on EETPL controller
//
// 5dec25: Met GROK alle overtollige "homebridge" publishes commented en dan verwijderd.
// 21oct23: Removed unnecessary ParticlePublish commands
// 19nov22: MOV1Lightson(); // Switched off automatic lighting (unnecessary)
// 15sep22: Newest Piettetech library (0.0.14) for DHT22. Replaced DS18B20 sensor
// 12sep22: Removed all unnecessary lines (Cleaned)
// ---------------------------------------
// PhotoniX shield v.3.0	I/O connections: (* = used)
//
// *D0/D1 - COM1/2 (I2C-SDA & SCL) = OLED display, I/O expander, Sunlight sensor (TSL2561)...
//  D2 - COM3 = 16-key touchpad (CLK) or TOUCH-COMMON
// *D3 - RoomSense T-BUS
// *D4 - OP5 = PIXEL-line
// *D5 - RoomSense MOV1
// *D6 - RoomSense TEMP/HUM
//  D7 - RoomSense GAS-DIG or DUST sensor (LED transmitter)
//  A0 - COM4 = 16-key touchpad (DATA1) = TOUCH-1
//  A1 - COM5 = DOTSTAR-line (Data) or 16-key touchpad (DATA2) = TOUCH-2
//  A2 - RoomSense GAS-ANA or DUST sensor (LED receiver)
// *A3 - RoomSense LIGHT => Put 22k series resistor with LDR (Not 5V tolerant!)
//  A4 - OP3 = PWM input for CO2
//  A5 - OP4 = MOV2 (Second light group), LCD-Reset or DOTSTAR-line (Clock)
//  A6 - OP1 (Not 5V!) = Thermostat switch to GND
//  A7 - OP2 = MOV2 operated dimmer pin or Alert light sensor (LDR) input.
//  TX/RX - COM6/7 = Serial comms
//  ---------------------------------------
//
// START Specific ROOM settings 1 (for accessories): "Room-EETPL"////////////////////////////////////////////////////////////

// *D3: ROOMSENSE connector: Select DS18B20 temperature sensors in use by uncommenting line(s):
//byte addrs0[1][8] = {{0x28,0xFF,0x69,0x29,0xA7,0x15,0x01,0x88}};  // ZITPL room: New Round RoomSenseBoX => New sensor!
byte addrs0[1][8] = {{0x28,0xFF,0xD5,0x13,0xA7,0x15,0x01,0xB5}};  // EETPL room: New Round RoomSenseBoX => New sensor!
// "Dummy-variables for unused pins" still needed for publishing the JSON string! (after removal of all related lines)
double CO2ppm = 0;  // Double to be published!
double Dust = 0;    // Double to be published!
double MOV2movement = 0; // Total movement over last 5 minutes
boolean TSTATon = 0;// Is the TSTAT ON?
boolean MOV2light = 0; // Is this light ON?
boolean STOREalert = 0;// Is STORE alert ON?
boolean STORElight = 0;// Is STORE alert LIGHT ON?
// STOP Specific ROOM settings 1 (for accessories): "Room-EETPL"/////////////////////////////////////////////////////////////

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
int Nightlevel = 1000; // Below this SUNLightlevel it is NIGHT! (broadcasted by another Photon running "I2C-Daylightsensor.ino")
boolean ItIsNight = 0; // This will be set to 1 or 0, depending on "checkDAY_NIGHT()" function. (Initialized as DAY)
String TIMEstatus = "Initialized as DAY!"; // Temporary string
boolean BedTime = 0;// BedTime status: Remotely created in function manual()

// *A3 - RoomSense LIGHT
int LIGHTpin = A3;
double ROOMLightlevel;// Double to be published!
int getLightInterval = 61 * 1000;
int getLightLastTime = millis() - getLightInterval;

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
int MOV1totaltime = 5 * 60 * 1000; // Time to reset movement summarization counter (5 min like Atomiot interval)
int MOV1totalLastTime;
String MOV1movstatus = "MOV1 empty"; // Initial setting
boolean MOV1occup = 0; // Is this occupied?
String MOV1lightstatus = "MOV1 light is OFF"; // Initial setting
boolean MOV1light = 0; // Is this light ON?
int MOV1LightONtime = 60 * 60 * 1000; // How long should the lights stay ON? (60 min)
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

  // Initialize Particle subscribe function:
  // Catching private events
  Particle.subscribe("Status-", EventDecoder, MY_DEVICES); // Listening for the event "Status-*" and, when it receives it, it will run the function EventDecoder()
  // Catching it's own device NAME
  Particle.subscribe("particle/device/name", DevNamereceiver); // Listening for the device name...
  delay(1000);

  // *A3 - RoomSense LIGHT
  pinMode(LIGHTpin,INPUT_PULLDOWN); // Light sensor. Input forced LOW. (If no sensor connected: 0)
  Particle.variable("ROOM_Light", &ROOMLightlevel, DOUBLE);

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
  // 27sep21: https://community.particle.io/t/surprising-issue-after-flashing-new-sketch-to-my-10-roomcontrollers/61194/18
  if (!strlen(device_name)) // If the variable has received contents...
      Particle.publish("particle/device/name"); // Ask the cloud (once) to send the device NAME!

  // General commands:
  // 1a. Check memory
  freemem = System.freeMemory();// For debugging: Track if memory leak exists...
  memPERCENT = (freemem/82944)*100; // Max 82944 kb

  if(memPERCENT < 30)
  {
    Particle.publish(stat_ALERT, "MEMORY LEAK in controller: Restarting!",60,PRIVATE);
    System.reset();
  }

  // 2. Check if it is day or night:
  checkDAY_NIGHT();

  // 3. REPORT Std Roomsense data to Homebridge:
  if ((millis()-HomebridgeLastTime) > HomebridgeInterval)
  {
    HomebridgeLastTime = millis(); // Reset reporting timer

    // NEW 10may20: Store the colour array values in simple variables:
    int Rval = rgb[0];
    int Gval = rgb[1];
    int Bval = rgb[2];

    // NEW 10may20
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

  // *A3 - RoomSense LIGHT = Std function!
  if ((millis()-getLightLastTime) > getLightInterval)
  {
    ReadLight();
    // Put eventual extra actions/calculations with LIGHT here...
    getLightLastTime = millis();
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
      //MOV1Lightson(); // Switched off automatic lighting (unnecessary)
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
    //MOV1Lightsoff(); // Switched off automatic lighting (unnecessary)
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
      //MOV1Lightsoff(); // Switched off automatic lighting (unnecessary)
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
  Particle.publish(stat_LIGHT, str,60,PRIVATE);

  // Turn on all related RGB lights to confirm the set color:
  //MOV1Lightson(); // Switched off automatic lights (unnecessary)
  return 1; // Feedback when successful...
}


// START Specific ROOM settings 3 (for LIGHTING): "Room-EETPL"////////////////////////////////////////////////////////////

// *D4 - PIXEL-Line: (Attention: First in string = 0!)
// POWERPIXEL CONTROL:
void MOV1Lightson() // Turn lights ON.
{
  if (!BedTime)
  {
    // Normal lights command
    // Powerpixel 0 = Select an option...
    DimUp(0, 0, 255, 5); // Wait = 5 = Fast, 20: Slow
    //strip.setPixelColor(0, strip.Color(255,255,255)); strip.show(); // Simple switching option: R= Spanlampen beneden, B= Spots onder palier (SIMPLE SWITCH ON = For testing)
    //strip.setPixelColor(0, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb()
    //DimUpGroups(0, 5); // Dimming option: Check "flicker" issues! PowerPiXel 0 = SpanBeneden, Min = 0, Steps = 10 (slow)
  }
  else
  {
    DimUp(0, 0, 255, 5); // Wait = 5 = Fast, 20: Slow
    //strip.setPixelColor(0, strip.Color(255,255,255)); strip.show(); // Simple switching option: R= Spanlampen beneden, B= Spots onder palier (SIMPLE SWITCH ON = For testing)
    //strip.setPixelColor(0, strip.Color(rgb[0], rgb[1], rgb[2])); strip.show(); // Use the web color picker function ledrgb()
    // Bedtime! Dimmed lights: Only staircase strip! Powerpixel 2
    // strip.setPixelColor(2, strip.Color(0,0,100)); strip.show(); // Light BLUE Simple switching option (For testing)
  }


  MOV1lightstatus = "MOV1 light is ON";
  //Particle.publish(stat_LIGHT, MOV1lightstatus, 60,PRIVATE);
  MOV1light = 1;
}

void MOV1Lightsoff() // Turn lights OFF per colour: R= Spanlampen beneden, G= Spots onder palier
{
  DimDown(0, 255, 0, 5); // Wait = 5 = Fast, 20: Slow
  //strip.setPixelColor(0, strip.Color(0,0,0)); strip.show(); // SIMPLE SWITCH OFF

  // Dimming option: Check "flicker" issues!
  //DimDownGroups(0, 5); // Steps = 10 (slow)

  MOV1lightstatus = "MOV1 light is OFF";
  //Particle.publish(stat_LIGHT, MOV1lightstatus, 60,PRIVATE);
  MOV1light = 0;
}


void MOV1demo() // One cycle dimming ON/OFF...
{
  MOV1Lightson();
  delay(5000);
  MOV1Lightsoff();
}
// STOP Specific ROOM settings 2 (for LIGHTING): "Room-EETPL"////////////////////////////////////////////////////////////




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
  if(command == "reportheat")
  {
    // ROOMSENSE box std functions
    sprintf(str, "Temp1:%2.1f",ROOMTemp1);
    Particle.publish(stat_HEAT, str,60,PRIVATE);
    sprintf(str, "Temp2:%2.1f Humid:%2.0f Dew:%2.1f Tout:%2.1f",ROOMTemp2,ROOMHumi,ROOMTdf,ROOMTout);
    if (ROOMTemp1 == ROOMTemp2)
    {
      Particle.publish(stat_HEAT, "Waiting OneWire: ROOMTemp1 = ROOMTemp2!",60,PRIVATE);
    }
    Particle.publish(stat_HEAT, str,60,PRIVATE);
    Particle.publish(stat_HEAT, outTEMPstatus, 60,PRIVATE);

    return 1002;
  }


  if(command == "reportlight")
  {
    sprintf(str, "SUNLIGHT:%2.0f",SUNLightlevel);
    Particle.publish(stat_LIGHT, str,60,PRIVATE);
    Particle.publish(stat_LIGHT, TIMEstatus, 60,PRIVATE);

    // ROOMSENSE box std functions
    sprintf(str, "Room Light:%2.0f",ROOMLightlevel);
    Particle.publish(stat_LIGHT, str,60,PRIVATE);

    // All RGB related lights: Selected RGB colour
    sprintf(str, "Colour:%d-%d-%d",rgb[0],rgb[1],rgb[2]);
    Particle.publish(stat_LIGHT, str,60,PRIVATE);
    // MOV1 Lights (= STD)
    Particle.publish(stat_LIGHT, MOV1lightstatus, 60,PRIVATE);

    return 1003;
  }


  if(command == "reportroom")
  {
    // GENERAL
    // Date/time stamp
    Particle.publish(stat_ROOM, Time.timeStr(), 60,PRIVATE); // eg: Wed May 21 01:08:47 2014
    Particle.publish(stat_ROOM, TIMEstatus, 60,PRIVATE);

    // System health data (JSON) => This is updated together with the homebridge data!
    Particle.publish(stat_ROOM, JSON_status,60,PRIVATE);

    // MOV1 Lights (= STD)
    sprintf(str, "MOV1movement:%2.0f",MOV1movement);
    Particle.publish(stat_ROOM, str,60,PRIVATE);
    Particle.publish(stat_ROOM, MOV1movstatus, 60,PRIVATE);

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

// EXTRA LIGHTS FOR EETPL:
// DIMMING RAMEN:
  if((command == "ramenlowon") || (command == "RaamL=1}")) // Second condition is for Homebridge
    {
      strip.setPixelColor(0, strip.Color(3,3,3)); strip.show(); // Simple switch ON
      return 3;
    }

  if((command == "ramenlowoff") || (command == "RaamL=0}")) // Second condition is for Homebridge
    {
      strip.setPixelColor(0, strip.Color(0,0,0)); strip.show(); // Simple switch ON
      return 0;
    }

  if((command == "ramenmedon") || (command == "RaamM=1}")) // Second condition is for Homebridge
    {
      strip.setPixelColor(0, strip.Color(10,10,10)); strip.show(); // Simple switch ON
      return 3;
    }

  if((command == "ramenmedoff") || (command == "RaamM=0}")) // Second condition is for Homebridge
    {
      strip.setPixelColor(0, strip.Color(0,0,0)); strip.show(); // Simple switch ON
      return 0;
    }

  if((command == "ramenhion") || (command == "RaamH=1}")) // Second condition is for Homebridge
    {
      strip.setPixelColor(0, strip.Color(60,60,60)); strip.show(); // Simple switch ON
      return 3;
    }

  if((command == "ramenhioff") || (command == "RaamH=0}")) // Second condition is for Homebridge
    {
      strip.setPixelColor(0, strip.Color(0,0,0)); strip.show(); // Simple switch ON
      return 0;
    }

// OTHERS:
  if((command == "tafelon") || (command == "Tafel=1}")) // Second condition is for Homebridge
    {
      //strip.setPixelColor(1, strip.Color(255,255,255)); strip.show(); // Simple switch ON
      DimUp(1, 0, 255, 5); // Wait = 5 = Fast, 20: Slow
      return 200;
    }

  if((command == "tafeloff") || (command == "Tafel=0}")) // Second condition is for Homebridge
  {
    //strip.setPixelColor(1, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    DimDown(1, 255, 0, 5); // Wait = 5 = Fast, 20: Slow
    return 2;
  }

  if((command == "bibon") || (command == "Bib=1}")) // Second condition is for Homebridge
    {
      //strip.setPixelColor(2, strip.Color(255,255,255)); strip.show(); // Simple switch ON
      DimUp(2, 0, 255, 5); // Wait = 5 = Fast, 20: Slow
      return 300;
    }

  if((command == "biboff") || (command == "Bib=0}")) // Second condition is for Homebridge
  {
    //strip.setPixelColor(2, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    DimDown(2, 255, 0, 5); // Wait = 5 = Fast, 20: Slow
    return 3;
  }

  if((command == "buroon") || (command == "Buro=1}")) // Second condition is for Homebridge
    {
      //strip.setPixelColor(3, strip.Color(255,255,255)); strip.show(); // Simple switch ON
      DimUp(3, 0, 255, 5); // Wait = 5 = Fast, 20: Slow
      return 400;
    }

  if((command == "burooff") || (command == "Buro=0}")) // Second condition is for Homebridge
  {
    //strip.setPixelColor(3, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    DimDown(3, 255, 0, 5); // Wait = 5 = Fast, 20: Slow
    return 4;
  }

  if((command == "bedon") || (command == "Bed=1}")) // Second condition is for Homebridge
    {
      //strip.setPixelColor(4, strip.Color(255,255,255)); strip.show(); // Simple switch ON
      DimUp(4, 0, 255, 5); // Wait = 5 = Fast, 20: Slow
      return 500;
    }

  if((command == "bedoff") || (command == "Bed=0}")) // Second condition is for Homebridge
  {
    //strip.setPixelColor(4, strip.Color(0,0,0)); strip.show(); // Simple switch OFF
    DimDown(4, 255, 0, 5); // Wait = 5 = Fast, 20: Slow
    return 5;
  }
// END EXTRA LIGHTS FOR EETPL

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

  Particle.publish(stat_ROOM, command,60,PRIVATE);  // If it does not match one of the above, publish the received string to see what it's "payload" was...
  return -1;// If none above
}
