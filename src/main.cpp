#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
#include "wifi.h"
#include "ntp.h"
#include "lamad.h"
#include "StringSplitter.h"
#include <MD_MAX72xx.h>
#include <SPI.h>
 
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   14    // or SCK
#define DATA_PIN  13    // or MOSI
#define CS_PIN    2     // or SS
#define CHAR_SPACING  1 // pixels between characters
#define BUF_SIZE  75
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN,  MAX_DEVICES);

// ntp stuff
NTP NTPclient;
#define CET 6 // central european time
#define NTP_SERVER "th.pool.ntp.org" 
//#define NTP_SERVER "ntp.egat.clo.th" 


#define CONFIGRESETBUTTON 4
bool reset_trigger = false;

// Praytime
String p_time[5] = {"21:21","21:22","22:00","23:00","23:30"};
int prevPlaying = 0;

time_t getNTPtime(void) {
  return NTPclient.getNtpTime();
}

// Print the text string to the LED matrix modules specified.
void printText(uint8_t modStart, uint8_t modEnd, char *pMsg) {
  uint8_t   state = 0;
  uint8_t   curLen = 0;
  uint16_t  showLen = 0;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  do {
    switch(state) {
      case 0: // Load the next character from the font table
        if (*pMsg == '\0') {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }
        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
      // !! deliberately fall through to next state to start displaying
      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);
        // done with font character, now display the space between chars
        if (curLen == showLen) {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3:	// display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

String getNow() {
  String h = String(hour());
  String m = String(minute());
  if (h.length() == 1) {
    h = "0" + h;
  }
  if (m.length() == 1) {
    m = "0" + m;
  }
	return  h + ':' + m;
}

void display_clock() {
  char message[BUF_SIZE];
	String msg = "  "+ getNow();
	msg.toCharArray(message, 50) ;
  printText(0, MAX_DEVICES-1, message);
}

int getMinute(String time) {
  int hour = time.substring(0,2).toInt();
  int minute = time.substring(3,5).toInt();
  //Serial.print(hour);Serial.print('-');Serial.println(minute);
  return hour*60 + minute;
}

void showNextPray() {
  char message[BUF_SIZE];
  int now = getMinute(getNow());
  
  String nextpray;  
  for (int i=0; i < 5 ; i++) {
    //Serial.print(getMinute(p_time[i]));Serial.print(">");Serial.println(now);
    if (getMinute(p_time[i]) > now) {
      nextpray = p_time[i];
      break;
    }
    else {
      nextpray = "";
    }
  }
  if (nextpray != "") {
	  String msg = "*"+ nextpray;
	  msg.toCharArray(message, 50) ;
    printText(0, MAX_DEVICES-1, message);
  }
}

void getLamad() {
  String praytime = getLamadData();  
  StringSplitter *splitter = new StringSplitter(praytime, ',', 6);
	Serial.println();
  for(int i = 0; i < 5; i++){
    String item = splitter->getItemAtIndex(i);
    Serial.println(item);
    p_time[i] =  item;
  }
}

// setup ----
void setup() {
	Serial.begin(9600);

  mx.begin();
	Serial.println(); Serial.println("boot"); Serial.println();

	// set flash button to config resetter
	pinMode(CONFIGRESETBUTTON, INPUT_PULLUP);

	// Set WiFi parameters explicitly: autoconnect, station mode 
	setupWiFi();

	ArduinoOTA.setHostname("matrixclock");
  ArduinoOTA.setPassword("matrixclockfirmware");
	ArduinoOTA.begin(); 

  NTPclient.begin(NTP_SERVER, CET);
  setSyncInterval(SECS_PER_HOUR);
  setSyncProvider(getNTPtime);

  getLamad();
}

void loop() {
  uint8_t	timeslice=now() % 60;
  
  int now = getMinute(getNow()); 

  if (now == 0 ) {
    NTPclient.begin(NTP_SERVER, CET);
    setSyncInterval(SECS_PER_HOUR);
    setSyncProvider(getNTPtime);
  }
 
	if ((timeslice >4) && (timeslice <8) ||  (timeslice >24) && (timeslice <28)) {
    showNextPray();
	}
	else {
		display_clock();
	}
  if ( now == getMinute(p_time[0]) && prevPlaying < now) {
    Serial.println("Play 1");
    prevPlaying = now;
  }
  else if ((now  == getMinute(p_time[1]) || now  == getMinute(p_time[2]) || now  == getMinute(p_time[3]) || now  == getMinute(p_time[4])) && prevPlaying < now) {
    Serial.println("Play 2");
    prevPlaying = now;
  }
  else if (now == getMinute("01:00") && prevPlaying < now) {
    Serial.println("get prayer time");
    getLamad();
    prevPlaying = now;
  }
/*
04:54
12:06
15:25
17:58
19:10
*/

	if (digitalRead(CONFIGRESETBUTTON)==LOW && false) { // & false to skip resetWiFi
		if (reset_trigger == false) {
			reset_trigger = true;
			Serial.println("Config reset button pressed, taking action");
			// reset wifi and ESP
			resetWiFi(); 
		}
	}

	ArduinoOTA.handle();
}
