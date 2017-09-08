//
//  Copyright (C) 2017 Ronald Guest <http://about.me/ronguest>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>
#include "Adafruit_NeoPixel.h"

// These are the paramters you might want to tweak
const unsigned long updateCycle = 750;    // How often to update the strip
int offTime = 21;                   // Time of day to turn off the LED strip
int onTime = 5;                     // Time of day to turn on the LED strip
TimeChangeRule usCDT = {"CDT", Second, dowSunday, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, dowSunday, Nov, 2, -360};
Timezone usCT(usCDT, usCST);

// The remaining items generally don't need to be changed
//
//US Central Time Zone (Chicago, Houston)
//static const char ntpServerName[] = "us.pool.ntp.org";
static const char ntpServerName[] = "time.nist.gov";
const int timeZone = 0;     // Using the Timezone library now which does it's own TZ and DST correction
TimeChangeRule *tcr;        //pointer to the time change rule, use to get the TZ abbrev
time_t local;
unsigned long lastUpdate;

////// Code and variables for NTP syncing
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
int ntpTime;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
boolean timeIsSet = false;

int hours = 0;                      // Track hours
//int minutes = 0;                    // Track minutes
//int seconds = 0;                    // Track seconds
//int dayOfWeek = 0;                  // Sunday == 1
int previousHour = 0;
boolean switchOn = true;            // Keep track of whether we are turned on or not

uint32_t Wheel(byte);
void turnOff();
void turnOn();
void rainbow();
time_t getNtpTime();
void sendNTPPacket(IPAddress&);
