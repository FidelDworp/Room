/* _Status_panel.ino = Status overview panel showing all room situations with DOTSTARs

14nov19 - Tested with v.1.4.2
26jan19 - Made boiler colours more realistic + Changed to easy arrangement: Starting pixel for every element (Boilers, rooms...) + Added INKOM, BandB, BADK

The status panel consists of a (high) number of addressable LEDs of which the colors are representing a particular status of the house, it's systems and it's rooms.
- The common house topics are: SUNlightlevel, Occupation, ...
- The systems (HVAC, WATER, ECO boiler, ...) are each designed specifically.
- The ROOMs are standardized to 11 LEDs. (The status PCB can contain 8 rows of 11 pixels...)

Functions of this sketch:

1) Control individual dotstar leds on the STATUS panel.
Thanks to @Scruffr, Particle community: https://community.particle.io/t/compile-errors-with-neopixel-library-and-dotstar-library-on-raspberry-pi/41363/43

Connections for hardware SPI:

APA102      Photon      RPi3B+
-----------------------------------
5v (R)      VIN         Pin 2
GND (Bk)    GND         Pin 6
Clock (Y)   A3 (CLK)    Pin 23 (SCK)
Data (G)    A5 (MOSI)   Pin 19 (MOSI)

* TO DO:

- Why is the controller always reported as offline? Too busy?
=> You cannot reach it for commands, reflashing etc...

- Standardize to 11 LEDs per room and make LED Nrs based on starting variables.

- How to modify to software SPI allowing to use any 2 pins for CLK & MOSI.
Reasons: 1) In case hardware SPI does not work on RPi, 2) To allow using unused pins on PhotoniX shield...
(Attention: SPI must be enabled at startup on RPi: add "dtparam=spi=on" to /boot/config.txt)

- Let the DOTSTAR intensity adapt to the room's light intensity: Use a variable for this, based on the received value
Example: setDOTSTAR(2,255,0,0,intensity);

- Originally, also TTS voice module and OLED display was combined with this DOTSTAR function.
For troubleshooting, I removed all functions for OLED and TTS voice module as it did not work together.
Try to use OLED display to show incoming messages and TTS to speak ALERT messages...

*/




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


// Element distribution: Common factors, Alerts, Boilers, Rooms... => Int values indicate the start of the series of LEDs.
// COMMON Status: 5 functions (Day/Night, Occupation, External movement (N,E,S,W), Heating ON, Ventilation ON...
// Common ALERTS: ...
int ALERTS = 0; // 0 = first LED
int COMMON = ALERTS +5; // Nr of previous group, incl black LED
// ROOMS: All rooms get 11 LEDs each for common functions
int INKOM = COMMON + 5; // Nr of previous group, incl black LED
int BADK = INKOM + 12; // Nr of previous group, incl black LED
int BandB = BADK + 12; // Nr of previous group, incl black LED
// BOILERS: All boilers get 9 LEDs each for common functions
int ECO_boiler = BandB + 12;// Nr of previous group, incl black LED
int SCH_boiler = ECO_boiler + 8;// Nr of previous group, incl black LED
int WON_boiler = SCH_boiler + 8;// Nr of previous group, incl black LED
// ACTIVITY INDICATOR: Last LED
int ACTIVE = WON_boiler + 8;// Nr of previous group, incl black LED


// Collecting data (If you create these in the function, it causes memory leak!)
int SUNLightlevel, SUN_R, SUN_G, SUN_B;

// ECO boiler (SOLAR & Woodfire energy)
float EQ1, EQ2, EQ3, EQ4, EQ5, EQtot;
int EAv, EQ1_R, EQ1_B, EQ2_R, EQ2_B, EQ3_R, EQ3_B, EQ4_R, EQ4_B, EQ5_R, EQ5_B;

// KS boiler (Kelder Schuur)
float SQ1, SQ2, SQ3, SQ4, SQ5, SQtot;
int SAv, SQ1_R, SQ1_B, SQ2_R, SQ2_B, SQ3_R, SQ3_B, SQ4_R, SQ4_B, SQ5_R, SQ5_B;
// Heat demand from KS boiler
float PowS;
int PowS_R, PowS_B;

// KW boiler (Kelder Woning)
float WQ1, WQ2, WQ3, WQ4, WQ5, WQtot;
int WAv, WQ1_R, WQ1_B, WQ2_R, WQ2_B, WQ3_R, WQ3_B, WQ4_R, WQ4_B, WQ5_R, WQ5_B;

// *DOTSTARs
const int LEDCOUNT = 144; // A full strip with 144 DOTSTARs/m
uint32_t pxBuffer[1 + LEDCOUNT + LEDCOUNT/16 + 1];
uint32_t setDOTSTAR(int px, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 31);

// Regularly, reset the ALErt DOTSTARs
int ALEResetInterval = 10*60*1000;// Every 10 minutes
uint32_t ALEResetLastTime = millis() - ALEResetInterval;




void setup()
{
  // *GENERAL Particle functions:
  Particle.function("Manual", manual);

  // Initialize Particle subscribe function:
  Particle.subscribe("Status", eventDecoder, MY_DEVICES); // Listen for eventnames starting with "Status", then run function eventDecoder()
  // To do: Can we receive ALL private messages...? (not only those starting with prefix "Status")

  // *DOTSTARs
  SPI.begin();
  SPI.setClockSpeed(8, MHZ);
  SPI.setBitOrder(MSBFIRST);
  // All DOTSTARs must be pre-set to "0xE0" once:
  for (int px = 0; px < LEDCOUNT; px++)
  pxBuffer[px] = 0x000000E0;
  // Initialize all "ALERT-Pixels" pale WHITE:
  setDOTSTAR(ALERTS+0,2,2,2,1); setDOTSTAR(ALERTS+1,2,2,2,1); setDOTSTAR(ALERTS+2,2,2,2,1);
} // End setup



void loop()
{
  // RESET ALERT-Pixels to very pale WHITE regularly: (Could be moved to a function later...)
  if ((millis()-ALEResetLastTime)>ALEResetInterval)
  {
    setDOTSTAR(ALERTS+0,2,2,2,1); setDOTSTAR(ALERTS+1,2,2,2,1); setDOTSTAR(ALERTS+2,2,2,2,1);
    ALEResetLastTime = millis(); // Reset ALErt timer
  }

 // Blinking activity indicator = "Red lantern..."
 setDOTSTAR(ACTIVE,0,0,20,1); delay(500); setDOTSTAR(ACTIVE,20,0,0,1); delay(500);
} // End loop



// FUNCTIONS:

// Event catcher: Interpret messages and display color of NEOPIXELs/DOTSTARs (and later also via OLED, Text-to-speech...)
void eventDecoder(const char *event, const char *data) // = Called when an event "Status*" is published...
{
  // INITIALIZE: Create "Name" , "Subject" and "Group" variables from received messages:
    // TO DO: Get the controller name also from the "Name"? (See roomsketch)

  // Explanation of command "strtok(strdup(event), "")": => Take first string until delimiter = ":" (If there is no : then the full string is copied!)
  char* Name = strtok(strdup(event), ""); // = Type of message
  char* Subject = strtok(strdup(data), ""); // = Message itself
  char* Group = strtok(strdup(data), ":"); // = Source from which it comes: Boiler, Room... (If "Subject" starts with a string followed by ":" => Create variable names, depending on the "Subject" name...)

  // COMMON:

  // (0) SUNLightlevel: (From I2C light sensor: 0-2000 LUX) => Used by all roomsketches to determine if it's Day or Night.
  if (strncmp(Group, "SUN", strlen("SUN")) == 0) // String = SUN
  {
    SUNLightlevel = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable

    // => Conditioning of SUNLightlevel for display: more YELLOW = day, more BLUE = NIGHT...
    SUN_R = constrain((SUNLightlevel/2), 0, 255); // In high light, more yellow
    SUN_G = constrain((SUNLightlevel/2), 0, 255); // In high light, more yellow
    SUN_B = constrain((255 - (SUNLightlevel/2)), 0, 255); // In low light, more blue
    // STATUS-Pixel 0 = set "dynamic" colour, depending on sunlight
    setDOTSTAR(COMMON+0,SUN_R,SUN_G,SUN_B,5);
  }

  // (1) Heatingmode: (from controller "HVAC")
  if (String(data) == "Heatingmode HOME") // Automatic, using available thermostats
  {
    setDOTSTAR(COMMON+1,0,255,0,5); // GREEN
  }
  if (String(data) == "Heatingmode OUT (= still Manual)") // Automatic, using Tout = Tcond + 2°C (safe for condens)
  {
    setDOTSTAR(COMMON+1,0,0,255,5); // BLUE
  }
  if (String(data) == "Heatingmode MANUAL") // Manually turn floor circuits ON/OFF
  {
    setDOTSTAR(COMMON+1,255,0,0,5); // RED
  }

// ROOMS: Convert the data to be catched per room into one string with all variables. Below version is temporarily...

  // Room "INKOM":
  // 1. ALERT status messages for controller "INKOM": ALWAYS show these immediately!
  if (String(event) == "Status-ALERT:INKOM")
  {
    // Turn the related ALERT Pixels ON:
    if (String(data) == "CONDENS DANGER!")
    {
      setDOTSTAR(INKOM+0,255,0,0,5); // RED
    }

    if (String(data) == "SMOKE or DUST!")
    {
      setDOTSTAR(INKOM+1,255,255,0,5); // YELLOW
    }
    if (String(data) == "HEAVY SMOKE or DUST!")
    {
      setDOTSTAR(INKOM+1,255,0,0,5); // RED
    }

    if (String(data) == "Room CO2 ppm HIGH")
    {
      setDOTSTAR(INKOM+2,255,255,0,5); // YELLOW
    }
    if (String(data) == "Room CO2 ppm too HIGH")
    {
      setDOTSTAR(INKOM+2,255,0,0,5); // RED
    }

    ALEResetLastTime = millis(); // Reset ALERT timer
  } // End alerts for room "INKOM"

  // 2. ROOM status messages for controller "INKOM":
  if (String(event) == "Status-ROOM:INKOM")
  {
    if (String(data) == "It is DAYTIME") //
    {
      setDOTSTAR(INKOM+3,255,255,0,3); // YELLOW
    }
    if (String(data) == "It is NIGHT")
    {
      setDOTSTAR(INKOM+3,0,0,100,3); // BLUE
    }

    if (String(data) == "Laser monitored doors open") //
    {
      setDOTSTAR(INKOM+4,255,0,0,5); // RED
    }
    if (String(data) == "Laser monitored doors closed") //
    {
      setDOTSTAR(INKOM+4,0,255,0,1); // GREEN
    }

    if (String(data) == "PIR1 in use") //
    {
      setDOTSTAR(INKOM+6,255,0,0,5); // RED
    }
    if (String(data) == "PIR1 empty")
    {
      setDOTSTAR(INKOM+6,0,255,0,1); // GREEN
    }

    if (String(data) == "PIR2 in use") //
    {
      setDOTSTAR(INKOM+8,255,0,0,5); // RED
    }
    if (String(data) == "PIR2 empty") //
    {
      setDOTSTAR(INKOM+8,0,255,0,1); // GREEN
    }
  } // End Status-ROOM for room "INKOM"

  // 3. LIGHT status messages for controller "INKOM":
  if (String(event) == "Status-LIGHT:INKOM")
  {
    if (String(data) == "Door lights ON") //
    {
      setDOTSTAR(INKOM+5,255,255,255,5); // WHITE
    }
    if (String(data) == "Door lights OFF") //
    {
      setDOTSTAR(INKOM+5,0,0,0,0); // OFF
    }

    if (String(data) == "PIR1 light is ON") //
    {
      setDOTSTAR(INKOM+7,255,255,255,5); // WHITE
    }
    if (String(data) == "PIR1 light is OFF") //
    {
      setDOTSTAR(INKOM+7,0,0,0,0); // OFF
    }

    if (String(data) == "PIR2 light is ON") //
    {
      setDOTSTAR(INKOM+9,255,255,255,5); // WHITE
    }
    if (String(data) == "PIR2 light is OFF") //
    {
      setDOTSTAR(INKOM+9,0,0,0,0); // OFF
    }
  } // End Status-LIGHT for room "INKOM"

  // 4. HEAT status messages for controller "INKOM":
  if (String(event) == "Status-HEAT:INKOM")
  {
    if (String(data) == "No Tstat heat demand") //
    {
      setDOTSTAR(INKOM+10,0,10,0,10); // Light green
    }
    if (String(data) == "Tstat heat demand") //
    {
      setDOTSTAR(INKOM+10,255,255,0,20); // YELLOW
    }
    if (String(data) == "No Tout heat demand (Humidity)") //
    {
      setDOTSTAR(INKOM+10,0,10,0,10); // Light green
    }
    if (String(data) == "Tout heat demand (Humidity)") //
    {
      setDOTSTAR(INKOM+10,255,0,0,20); // RED
    }
  } // End Status-HEAT for room "INKOM"




  // 2) ECO-boiler energy in the 5 layers. Example of message: "ECO: 0.9,0.5,0.3,-0.2,-0.7=0.78(36)"
  if (strncmp(Subject, "*ECO", strlen("*ECO")) == 0) // String = ""*ECO": There are other messages also starting with "ECO", therefore distinguish this one with asterisk...
  {
    EQ1 = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable
    EQ2 = atof(strtok(NULL, ","));
    EQ3 = atof(strtok(NULL, ","));
    EQ4 = atof(strtok(NULL, ","));
    EQ5 = atof(strtok(NULL, "=")); // Convert next string until delimiter = "=" to double variable
    EQtot = atof(strtok(NULL, "(")); // TOTAL boiler water energy (kWh); Convert next string until delimiter = "(" to double variable
    EAv = atoi(strtok(NULL, ")"));   // Convert next string until delimiter = ")" to integer variable

    // => Conditioning of EQ1 energy for display on DOTSTARs: set "dynamic" colour: more RED = hot, more BLUE = cold...
    int offset = 1.3; // = to avoid negative figures: -1.3 kW is 'theoretically' the lowest possible energy level in this boiler @ 20°C in every layer)
    int factor = 83; // To reach pure RED @ 60°C

    EQ1_R = constrain(((EQ1+offset)*factor), 0, 255); // hot = more RED
    EQ1_B = constrain((255 - ((EQ1+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(ECO_boiler+4,EQ1_R,0,EQ1_B,3);

    EQ2_R = constrain(((EQ2+offset)*factor), 0, 255); // hot = more RED
    EQ2_B = constrain((255 - ((EQ2+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(ECO_boiler+3,EQ2_R,0,EQ2_B,3);

    EQ3_R = constrain(((EQ3+offset)*factor), 0, 255); // hot = more RED
    EQ3_B = constrain((255 - ((EQ3+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(ECO_boiler+2,EQ3_R,0,EQ3_B,3);

    EQ4_R = constrain(((EQ4+offset)*factor), 0, 255); // hot = more RED
    EQ4_B = constrain((255 - ((EQ4+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(ECO_boiler+1,EQ4_R,0,EQ4_B,3);

    EQ5_R = constrain(((EQ5+offset)*factor), 0, 255); // hot = more RED
    EQ5_B = constrain((255 - ((EQ5+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(ECO_boiler+0,EQ5_R,0,EQ5_B,3);

    // => Use EQtot to see if boiler water is OK for a shower or not:
    if (EQtot >= 0)
    {
      setDOTSTAR(ECO_boiler+5,0,255,0,(EQtot*2)); // "Dynamic" GREEN = OK for shower (Proportional to EQtot)
    }
    else
    {
      setDOTSTAR(ECO_boiler+5,0,0,255,1); // pale BLUE = too cold for shower!
    }
  }

  // 3) KS-boiler (schuur) energy in the 5 layers. Example of message: "KS: -2.7,-2.2,-2.2,-2.2,-2.7=-11.97( 0)"
  if (strncmp(Subject, "*KS", strlen("*KS")) == 0) // String = *KS
  {
    SQ1 = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable
    SQ2 = atof(strtok(NULL, ","));
    SQ3 = atof(strtok(NULL, ","));
    SQ4 = atof(strtok(NULL, ","));
    SQ5 = atof(strtok(NULL, "=")); // Convert next string until delimiter = "=" to double variable
    SQtot = atof(strtok(NULL, "(")); // TOTAL boiler water energy (kWh); Convert next string until delimiter = "(" to double variable
    SAv = atoi(strtok(NULL, ")"));   // Convert next string until delimiter = ")" to integer variable

    // => Conditioning of KQ1 energy for display on DOTSTARs: set "dynamic" colour: more RED = hot, more BLUE = cold...
    int offset = 1.9; // = to avoid negative figures: -1.9 is 'theoretically' the lowest possible energy level)
    int factor = 104; // To reach pure RED @ 40°C

    SQ1_R = constrain(((SQ1+offset)*factor), 0, 255); // hot = more RED
    SQ1_B = constrain((255 - ((SQ1+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(SCH_boiler+4,SQ1_R,0,SQ1_B,3);

    SQ2_R = constrain(((SQ2+offset)*factor), 0, 255); // hot = more RED
    SQ2_B = constrain((255 - ((SQ2+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(SCH_boiler+3,SQ2_R,0,SQ2_B,3);

    SQ3_R = constrain(((SQ3+offset)*factor), 0, 255); // hot = more RED
    SQ3_B = constrain((255 - ((SQ3+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(SCH_boiler+2,SQ3_R,0,SQ3_B,3);

    SQ4_R = constrain(((SQ4+offset)*factor), 0, 255); // hot = more RED
    SQ4_B = constrain((255 - ((SQ4+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(SCH_boiler+1,SQ4_R,0,SQ4_B,3);

    SQ5_R = constrain(((SQ5+offset)*factor), 0, 255); // hot = more RED
    SQ5_B = constrain((255 - ((SQ5+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(SCH_boiler+0,SQ5_R,0,SQ5_B,3);

    // => Use SQtot to see if boiler water is OK for a floor heating or not:
    if (SQtot >= 0)
    {
      setDOTSTAR(SCH_boiler+5,0,255,0,(SQtot*2)); // "Dynamic" GREEN = OK for floor heating (Proportional to SQtot)
    }
    else
    {
      setDOTSTAR(SCH_boiler+5,0,0,255,1); // pale BLUE = too cold for floor heating!
    }
  }

  // Heat demand from KS-boiler (schuur)
     // Example of message: "Heat Demand: Current= 6.60, Hourly= 4.06 kWh, Total= 108.57 kWh"
  if (strncmp(Subject, "Heat Demand", strlen("Heat Demand")) == 0) // Catch messages starting with string = "Heat Demand"
  {
    PowS = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable
    // => Display heat demand with 2nd DOTSTAR above boiler:
    PowS_R = constrain((PowS*50), 0, 255); // high demand = more RED (6kW = 100% RED)
    PowS_B = constrain((255 - (PowS*50)), 0, 255); // low demand = more BLUE (0kW = 100% RED)
    setDOTSTAR(SCH_boiler+6,PowS_R,0,PowS_B,3);
  }


  // 5) KW-boiler (woning) energy in the 5 layers. Example of message: "KW: 0.9,0.5,0.3,-0.2,-0.7=0.78(36)"
  if (strncmp(Subject, "*KW", strlen("*KW")) == 0) // String = *KW
  {
    WQ1 = atof(strtok(NULL, ",")); // Convert next string until delimiter = "," to double variable
    WQ2 = atof(strtok(NULL, ","));
    WQ3 = atof(strtok(NULL, ","));
    WQ4 = atof(strtok(NULL, ","));
    WQ5 = atof(strtok(NULL, "=")); // Convert next string until delimiter = "=" to double variable
    WQtot = atof(strtok(NULL, "(")); // TOTAL boiler water energy (kWh); Convert next string until delimiter = "(" to double variable
    WAv = atoi(strtok(NULL, ")"));   // Convert next string until delimiter = ")" to integer variable

    // => Conditioning of KQ1 energy for display on DOTSTARs: set "dynamic" colour: more RED = hot, more BLUE = cold...
    int offset = 1.9; // = to avoid negative figures: -1.9 is 'theoretically' the lowest possible energy level)
    int factor = 104; // To reach pure RED @ 40°C

    WQ1_R = constrain(((WQ1+offset)*factor), 0, 255); // hot = more RED
    WQ1_B = constrain((255 - ((WQ1+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(WON_boiler+4,WQ1_R,0,WQ1_B,3);

    WQ2_R = constrain(((WQ2+offset)*factor), 0, 255); // hot = more RED
    WQ2_B = constrain((255 - ((WQ2+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(WON_boiler+3,WQ2_R,0,WQ2_B,3);

    WQ3_R = constrain(((WQ3+offset)*factor), 0, 255); // hot = more RED
    WQ3_B = constrain((255 - ((WQ3+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(WON_boiler+2,WQ3_R,0,WQ3_B,3);

    WQ4_R = constrain(((WQ4+offset)*factor), 0, 255); // hot = more RED
    WQ4_B = constrain((255 - ((WQ4+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(WON_boiler+1,WQ4_R,0,WQ4_B,3);

    WQ5_R = constrain(((WQ5+offset)*factor), 0, 255); // hot = more RED
    WQ5_B = constrain((255 - ((WQ5+offset)*factor)), 0, 255); // cold = more BLUE
    setDOTSTAR(WON_boiler+0,WQ5_R,0,WQ5_B,3);

    // => Use WQtot to see if boiler water is OK for a floor heating or not:
    if (WQtot >= 0)
    {
      setDOTSTAR(WON_boiler+5,0,255,0,(WQtot*2)); // "Dynamic" GREEN = OK for floor heating (Proportional to WQtot)
    }
    else
    {
      setDOTSTAR(WON_boiler+5,0,0,255,1); // pale BLUE = too cold for floor heating!
    }
  }

  free(Name); free(Subject); free(Group); // ATTENTION:"free" these variables every time to avoid memory leak!

} // END eventDecoder() function



// * When this function is called the command is assembled from the parameters and sent to the DOTSTARs by SPI bus:
uint32_t setDOTSTAR(int px, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
  uint32_t color = 0;

  color |= (brightness <<   0) | 0xE0;
  color |= (b          <<   8);
  color |= (g          <<  16);
  color |= (r          <<  24);

  pxBuffer[px] = color;
  SPI.transfer(pxBuffer, NULL, sizeof(pxBuffer), NULL);
  return color;
}




// External control Function(s):

int manual(String command) // = Particle.function to remote control manually. Can also be called from the loop(): ex = manual("report");
{
  if(command == "report")
  {
    // 1.GENERAL: Time & date string
    Particle.publish("Status-ROOM", Time.timeStr(), 60,PRIVATE); delay(1000); // eg: Wed May 21 01:08:47 2014

    // WiFi reception
    wifiRSSI = WiFi.RSSI();
    wifiPERCENT = wifiRSSI+120;
    sprintf(str, "wifiRSSI + wifiPERCENT: %2.0f + %2.0f",wifiRSSI,wifiPERCENT); Particle.publish("Status-ROOM", str,60,PRIVATE); delay(1000);

    // Memory monitoring
    freemem = System.freeMemory();// For debugging: Track if memory leak exists...
    memPERCENT = (freemem/51312)*100;
    sprintf(str, "Mem+Pct:%2.0f+%2.0f",freemem,memPERCENT); Particle.publish("Status-MEMORY", str,60,PRIVATE); delay(1000);

    // Publish received heating data to see if they are well received:
    sprintf(str, "ECO boiler:%2.1f,%2.1f,%2.1f,%2.1f,%2.1f=%2.1f",EQ1,EQ2,EQ3,EQ4,EQ5,EQtot);
    Particle.publish("TEST_capture", str,60,PRIVATE); delay(1000);

    sprintf(str, "SCH boiler:%2.1f,%2.1f,%2.1f,%2.1f,%2.1f=%2.1f",SQ1,SQ2,SQ3,SQ4,SQ5,SQtot);
    Particle.publish("TEST_capture", str,60,PRIVATE); delay(1000);

    sprintf(str, "WON boiler:%2.1f,%2.1f,%2.1f,%2.1f,%2.1f=%2.1f",WQ1,WQ2,WQ3,WQ4,WQ5,WQtot);
    Particle.publish("TEST_capture", str,60,PRIVATE); delay(1000);

    sprintf(str, "Heat demand:%2.1f kW",PowS);
    Particle.publish("TEST_capture", str,60,PRIVATE); delay(1000);


    return 1000;
  }

  if(command == "reset") // Remotely RESET the controller with this command...
  {
    System.reset();
    return 10000;
  }

  Particle.publish("Status-ROOM", command,60,PRIVATE); delay(1000); // If it does not match one of the above, publish the received string to see what it's "payload" was...
  return -1;// If none above
}
