/* -ROOM_R7-ZITPL.ino : Operate CLOCK and RELAYS on the TV wall

5dec25: Reflash & update OS.
31Oct23: Added "Summer" & "Winter" commands for clock: beginDST() + endDST()
19jul23: Added "Manual" commands for SPOTS + TV BACKLIGHTS (2 relays: D2 + A0)
25apr23: Added "Manual" commands: Zomer en wintertijd setting, Hi, Medium & Low light, Report for development.
=> Gebruikt in onze living te Zarlardinge
2019: PiXelClock.ino = A real time clock with 12 neopixels. (a Filip Delannoy development)
 = Geleerd aan Leon: Hij kreeg een mee naar huis: Photon + 12 pixels

*/

// *GENERAL
 SYSTEM_MODE(AUTOMATIC);

// *D4 - PIXEL-DI => Lights
#include <neopixel.h>
 #define PIXEL_COUNT 12
 #define PIXEL_PIN D4
 #define PIXEL_TYPE WS2812
 Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

 // Store time related elements for the clock:
 double Hour, Minute, Second;
 int HourLED, MinuteLED, SecondLED;
 int Strength = 100; // Std strength = medium
 char JSON[400]; // Publish variables in one string

 // Pins to operate the two relays:
 int relay1 = D2;
 int relay2 = A0;



void setup()
{
  // GENERAL Particle functions:
  Time.zone(2.0); // Brussels time = GMT+2 (Summer)
  Particle.function("Manual", manual);

  // *D4 - PIXEL-DI:
  strip.begin(); // Initialize strip
  strip.show();

  // Pins to operate the two relays:
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  // Turn OFF both relays initially:
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
}



void loop()
{
  // Turn all LEDS OFF before next cycle
  for(int i=0;i<12;i++)
  {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }

  // Store actual time in variables and display them with the LEDs:
  Second = (Time.second())/5;
  if (Second ==0)
  {
    Second = 12;
  }
  SecondLED = Second-1;
  strip.setPixelColor(SecondLED, strip.Color(0,Strength,0)); // G,R,B => Omie: (0,5,0)

  Minute = (Time.minute())/5;
  if (Minute ==0)
  {
    Minute = 12;
  }
  MinuteLED = Minute-1;
  strip.setPixelColor(MinuteLED, strip.Color(0,0,Strength)); // G,R,B => Omie: (10,10,0)

  Hour = Time.hourFormat12();
  if (Hour ==0)
  {
    Hour = 12;
  }
  HourLED = Hour-1;
  strip.setPixelColor(HourLED, strip.Color(Strength,Strength,0)); // G,R,B => Omie: (10,10,0)
  strip.show();

  // Update the JSON string:
  snprintf(JSON,400,"{\"H\":%d,\"M\":%d,\"S\":%d,\"Hour\":%.2f,\"HourLED\":%d,\"Minute\":%.2f,\"MinuteLED\":%d,\"Second\":%.2f,\"SecondLED\":%d}",Time.hourFormat12(),Time.minute(),Time.second(),Hour,HourLED,Minute,MinuteLED,Second,SecondLED);
}




int manual(String command) // = Particle.function to remotely control. Can also be called from the loop(): ex = manual("Lighton");
{
  if(command == "h")
  {
    Strength = 255;
    return 2;
  }

  if(command == "m")
  {
    Strength = 60;
    return 2;
  }

  if(command == "l")
  {
    Strength = 10;
    return 2;
  }

  if(command == "summer")
  {
    Time.zone(2.0); // Brussels time = GMT+2
    return 2;
  }

  if(command == "winter")
  {
    Time.zone(1.0); // Brussels time = GMT+1
    return 1;
  }

  if((command == "muurspotson") || (command == "Muur=1}")) // Second condition is for Homebridge
  {
    digitalWrite(relay1, LOW);
    return 1;
  }

  if((command == "muurspotsoff") || (command == "Muur=0}")) // Second condition is for Homebridge
  {
    digitalWrite(relay1, HIGH);
    return 1;
  }

  if((command == "tvbackon") || (command == "TVback=1}")) // Second condition is for Homebridge
  {
    digitalWrite(relay2, LOW);
    return 1;
  }

  if((command == "tvbackoff") || (command == "TVback=0}")) // Second condition is for Homebridge
  {
    digitalWrite(relay2, HIGH);
    return 1;
  }

  if(command == "report")
  {
    Particle.publish("PixelClock", JSON,60,PRIVATE);
    return 3;
  }

  if(command == "reset") // You can remotely RESET the photon with this command... (It won't reset the IO-eXbox!)
  {
    System.reset();
    return -10000;
  }

  Particle.publish("PixelClock", command,60,PRIVATE);  // If it does not match one of the above, publish the received string to see what it's "payload" was...
  return -1;// If none above
}
