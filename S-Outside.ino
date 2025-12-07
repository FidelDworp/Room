// -S-Outside.ino = Operating a "Sunlight sensor", connected to the I2C connector of a PhotoniX shield.
// Purpose: The TSL2561 is ideal as "solar" sensor to determine for all rooms if it is day or night. The lighting system response is based on that. Any Room controller could play this role.
//
// Attention: In direct sunlight (> 40000 lux), the sensor fails! (We substitute the result by "40000")
// Note: If you leave the TSL2561 ADDR connection 'floating', the default addr = 0x39.
//
// Revisions:
//- 5dec25: Change reset threshold in the morning + include real lux in broadcast string
//- 26nov25: Correction in revised sketch!
//- 25nov25: Added automatic reset function to AUTO mode in the morning and evening. (Grok)
//- 3nov25: Removed Homebridge commands, Removed delay commands after publish commands, Stuur manueel "3000 LUX" bij daymode "PRETEND IT'S DAY!" => ECO solar werkt dan wel!
//- 18apr23: Pretend daytime = 300 lux. (Was 1000 lux before)
//- 25sep21: Reduced message Interval (1min)
//-  4apr23: Increased message Interval (30s)


#include <tsl2561.h>

// GENERAL:
STARTUP(WiFi.selectAntenna(ANT_INTERNAL));
SYSTEM_THREAD(ENABLED);

// String for publishing variables
char str[255];

// *D0 & D1 - I2C: TSL2561 sensor
TSL2561 tsl(TSL2561_ADDR);// Instanciate a TSL2561 object with I2C address = 0x39
double sunlight;
uint32_t sunlight_int;
uint16_t integrationTime;
bool autoGainOn;
bool operational;
char tsl_status[21] = "void";
uint8_t error_code;
uint8_t gain_setting;
int getSunInterval = 30 * 1000;
int getSunLastTime = millis() - getSunInterval;
int daymode = 2;// Daymode variable: Set remotely to simulate Day (1), Night(0) or Auto(3 =publish current daylight)

// Automatic reset to AUTO in the morning and evening:
bool isCurrentlyDark = true;                         // start in "nacht"-state
const double ENTER_DAY_THRESHOLD   = 15.0;          // zonsopgang-reset bij ≥ 180 lux (vroeg genoeg, maar geen reset op superdonkere dagen)
const double ENTER_NIGHT_THRESHOLD = 5.0;           // zonsondergang-reset bij ≤ 80 lux (voorkomt reset bij schemer alleen) =>  (moet lager zijn dan bovenstaande!)



void setup()
{
  // GENERAL Particle functions:
  Particle.function("Manual", manual);

  // *D0 & D1 - I2C: TSL2561 sensor
  error_code = 0;
  operational = false;

  Particle.variable("TSL-status", tsl_status);
  Particle.variable("Sunlight", sunlight);

    if (tsl.begin()) // connecting to light sensor device
    {
      strcpy(tsl_status,"TSL2561 found");
    }
    else
    {
      strcpy(tsl_status,"TSL2561 NOT found!");
    }

    if(!tsl.setTiming(false,1,integrationTime)) // Setting the sensor: gain x1 and 101ms integration time
    {
      error_code = tsl.getError();
      strcpy(tsl_status,"SetTiming Error");
      return;
    }

    if (!tsl.setPowerUp())
    {
      error_code = tsl.getError();
      strcpy(tsl_status,"PowerUP Error");
      return;
    }

    operational = true; // Initialized!
    strcpy(tsl_status,"Init OK");
}





// For automatic reset to AUTO in the morning and evening 5dec25 ===
void checkForAutoReset()
{
  if (!operational) return;

  if (isCurrentlyDark)
  {
    if (sunlight >= ENTER_DAY_THRESHOLD)
    {
      isCurrentlyDark = false;
      daymode = 2;
      Particle.publish("Status-LIGHTS", "RESET TO AUTO (sunrise)", 60, PRIVATE);
    }
  }
  else
  {
    if (sunlight <= ENTER_NIGHT_THRESHOLD)
    {
      isCurrentlyDark = true;
      daymode = 2;
      Particle.publish("Status-LIGHTS", "RESET TO AUTO (sunset)", 60, PRIVATE);
    }
  }
}




void loop()
{
  if ((millis() - getSunLastTime) > getSunInterval)
  {
    GetLux();
    checkForAutoReset(); // For automatic reset to AUTO in the morning and evening ===
    getSunLastTime = millis();
  }
}




void GetLux()
{
  uint16_t broadband, ir; // update exposure settings display vars
  if (tsl._gain)
  gain_setting = 16;
  else
  gain_setting = 1;

  if (operational) // device operational, update status vars
  {
    strcpy(tsl_status,"TSL2561 OK");
    if(!tsl.getData(broadband,ir,autoGainOn))  // get raw data from sensor
    {
      error_code = tsl.getError();
      strcpy(tsl_status,"Saturated ???");
      operational = false;
    }

    if(!tsl.getLux(integrationTime,broadband,ir,sunlight)) // compute sunlight value in lux
    {
      error_code = tsl.getError();
      strcpy(tsl_status,"GetLux Error");
      operational = false;
    }

    if(!tsl.getLuxInt(broadband,ir,sunlight_int)) // try the integer based calculation
    {
      error_code = tsl.getError();
      strcpy(tsl_status,"Get LuxInt Error");
      operational = false;
    }
  }
  else // device not set correctly
  {
      strcpy(tsl_status,"Operation Error");
      sunlight = sunlight_int = 40000; // Substitute to a "daytime value" (to avoid lights turning on!)
      tsl.setPowerDown(); // trying a fix: power down the sensor

      if (tsl.begin()) // re-init the sensor
      {
        tsl.setPowerUp(); // power up
        tsl.setTiming(tsl._gain,1,integrationTime); // re-configure: try to go back normal again
        operational = true;
      }
  }

  // Publish variables in one string with SPRINTF: Select one of 3 possibilities (day, night or auto) with the Particle App on smartphone...
  if (daymode == 0)
  {
    sprintf(str, "SUN: %2.0f LUX (real %2.0f)", 0.00, sunlight);   // PRETEND IT'S NIGHT!
  }
  else if (daymode == 1)
  {
    sprintf(str, "SUN: %2.0f LUX (real %2.0f)", 3000.00, sunlight); // PRETEND IT'S DAY!
  }
  else // daymode == 2
  {
    sprintf(str, "SUN: %2.0f LUX (real %2.0f)", sunlight, sunlight); // AUTO – echte waarde
  }
  Particle.publish("Status-LIGHTS", str, 60, PRIVATE);
}





int manual(String command) // = Particle.function to remote control manually. Can also be called from the loop(): ex = manual("Lighton");
{
    if(command == "auto") // Default daylight broadcasting
    {
      daymode = 2; // Set variable for AUTO
      return 2;
    }

    if(command == "day") // FORCE DAY broadcast
    {
      daymode = 1; // Set variable for DAY
      return 1;
    }

    if(command == "night") // FORCE NIGHT broadcast
    {
      daymode = 0; // Set variable for NIGHT
      return 0;
    }

    if(command == "Reset") // You can remotely RESET the photon with this command
    {
      System.reset();
      return -10000;
    }

    Particle.publish("Status-ROOM", command,60,PRIVATE); // If it does not match one of the above, publish the received string to see what it's "payload" was...
    return -1;// If none above
}
