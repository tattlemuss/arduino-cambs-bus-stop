// Libraries
#include <Adafruit_CC3000.h>
#include <Servo.h>
#include <SPI.h>

#include "bus_parse.h"
#include "wifi_settings.h"



#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab.
#define WEBSITE      "www.cambridgeshirebus.info"
#define WEBPAGE      "/Text/WebDisplay.aspx?stopRef=0500SHIST010&stopName=Barrowcrofts"

#ifdef DEBUG
#define SERIAL(x)
#else
#define SERIAL(x)    Serial.println(x)
#endif

// Definition of the Arduino Servo object
// NOTE: this will run code before main()
Servo myServo;

// Static Definition of the wifi controller
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
                                         ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (c
    alled automatically
            on startup)
*/
/**************************************************************************/

uint32_t ip;

void setup_wifi(void)
{
  Serial.println(F("Hello, CC3000!\n")); 
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
  // listSSIDResults();
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  ip = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }
}

void read_webpage()
{
  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(WEBPAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }

  SERIAL(F("-------------------------------------"));
  BusParser parse;
  parse.init();
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      
      // Pass to the html parser
      parse.add(c);
      lastRead = millis();
    }
  }
  www.close();

  if (parse.m_numTimes > 0)
  {
    // Point to soonest bus
    uint32_t busOff = GetTimeDiff(parse.m_nowTime, parse.m_time[0]);
    
    uint32_t potVal = 160;
    
    if (busOff < 20)
      potVal = ((uint32_t)busOff) * 180 / 20;
    SERIAL(busOff);
    SERIAL(potVal);
    myServo.attach(9);
    myServo.write(potVal);
    delay(2000);    // allow time for servo
    myServo.detach();
  }
}

void setup()
{
  Serial.begin(9600);
  setup_wifi();
}

int time = 0;
void loop()
{
    SERIAL(F("Try read page"));  
    read_webpage();
    time ++;
    // Wait for a minute
    delay(1000 * 10);
    delay(1000 * 10);
    delay(1000 * 10);
}
