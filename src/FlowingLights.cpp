//
//  Copyright (C) 2017 Ronald Guest <http://about.me/ronguest>

#include <Arduino.h>
#include "config.h"

// Configure based on your Neopixel strip characteristics
#define PIXEL_PIN     5
#define PIXEL_COUNT   30
#define PIXEL_TYPE    NEO_GRB + NEO_KHZ800
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup() {
  Serial.begin(115200);
  // Get current time using NTP
  Udp.begin(localPort);
  ntpTime = getNtpTime();
  if (ntpTime != 0) {
    setTime(ntpTime);
    timeIsSet = true;
  } else {
    Serial.println("Failed to set the initial time");
  }
  pixels.begin();
}

void loop() {
  time_t local = usCT.toLocal(now(), &tcr);
  hours = hour(local);

  // Check if we should update LED strip
  if (((millis() - lastUpdate) > updateCycle) || !timeIsSet) {
    if (switchOn) {
      rainbow();
    }
    lastUpdate = millis();
  }

  // When the hour changes check to see if it is time to turn on/off
  // We also sync with NTP time once an hour
  if (hours != previousHour) {
    previousHour = hours;
    // Sync time at onTime every day, or if time failed set on boot
    if((hours == onTime) || !timeIsSet) {
      // Try an NTP time sync so we don't stray too far
      ntpTime = getNtpTime();
      if (ntpTime != 0) {
        setTime(ntpTime);
        timeIsSet = true;
      } else {
        Serial.println("NTP sync failed");
      }
    }

    if (hours == offTime) {
      turnOff();
    }
    // Same if it is time to turn the LEDs on
    if (hours == onTime) {
      turnOn();
    }
  }
}

// Each call to rainbow runs through an update of each pixel using the current cycleColor
uint16_t cycleColor = 0;
void rainbow() {
  if (cycleColor == 0) {
    Serial.println("Start new color cycle");
  }
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, Wheel((i+cycleColor) & 255));
  }
  pixels.show();
  cycleColor++;
  if (cycleColor > 255) cycleColor = 0;
}

// Turn off the LEDs
void turnOff() {
  Serial.println("Turn off");
  switchOn = false;
  for (int i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, 0);
  }
  pixels.show();
}

// Turn on the LEDs
void turnOn() {
  Serial.println("Turn on");
  switchOn = true;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address

  if(WiFi.status() == WL_CONNECTED) {
    while (Udp.parsePacket() > 0) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // discard any previously received packets, I made this change not sure if needed though
    }
    Serial.print("Transmit NTP Request ");
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);
    Serial.print(ntpServerName);
    Serial.print(": ");
    Serial.println(ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) { // Extending wait from 1500 to 2-3k seemed to avoid the sync problem, but now it doesn't help
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
        Serial.println("Receive NTP Response");
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      }
    }
    Serial.println("No NTP Response");
    return 0; // return 0 if unable to get the time
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
