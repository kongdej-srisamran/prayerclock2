/* Prayer Clock Version 2 by s.kongdej nov 2019 */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
#include "wifi.h"
#include "ntp.h"
#include "StringSplitter.h"
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <MD_YX5300.h>
#include <ArduinoJson.h>
#include "StringSplitter.h"
#include <ESP8266HTTPClient.h>

char ntpserver[NTP_LEN];
char apitoken[APITOKEN_LEN];
String host = "http://www.muslimthaipost.com/prayertimes/solaat.php?";

// Connections for serial interface to the YX5300 module
const uint8_t RX = 5;    // D1 connect to TX of MP3 Player module
const uint8_t TX = 4;    // D2 connect to RX of MP3 Player module
const uint8_t PLAY_FOLDER = 1;
MD_YX5300 mp3(RX, TX);   // Define global variables

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4
#define CLK_PIN       14    // D5 or SCK
#define DATA_PIN      0     // D3 or MOSI
#define CS_PIN        2     // D4 or SS
#define CHAR_SPACING  1     // pixels between characters
#define BUF_SIZE  75
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN,  MAX_DEVICES);

// ntp stuff
NTP NTPclient;
#define CET 7 // central european time
#define CONFIGRESETBUTTON    16 // D0
bool reset_trigger = false;

// Praytime
String p_time[5] = {"13:22","13:30","13:40","13:50","13:55"};
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
	String msg = " "+ getNow();
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

String getLamadData(){
	String praytime;
	String url = String(host)+String(apitoken);
	String line;
	//Serial.println(apitoken);
	Serial.println(String("GET " + url)); // debug info

	if (WiFi.status() == WL_CONNECTED) // Check WiFi connection status
	{
		HTTPClient http;  // Declare an object of class HTTPClient
		http.begin(url);  // Specify request destination
		int httpCode = http.GET(); //Send the request
		if (httpCode > 0) // Check the returning code
		{
			line = http.getString();   // Get the request response payload
		}
		else
		{
			Serial.println("HTTP response code is no good");
			http.end();
			return "0";
		}
		http.end();   //Close connection
		//Serial.print("Response :"); Serial.println(line); // debug info

		//drawStringClr(0,font,"link up");
		//String test = "<html>x.<table>x.<tr><td>01:00 x.</td></tr><tr><td>02:00 x.</td></tr><tr><td>03:00 x.</td></tr><tr><td>04:00 x.</td></tr><tr><td>05:00 x.</td></tr></table>";
		StringSplitter *splitter = new StringSplitter(line, '.', 8);
		//int itemCount = splitter->getItemCount();
		
		//Serial.println();

		for(int i = 2; i < 7; i++){
			String item = splitter->getItemAtIndex(i);
			String t =  item.substring(item.length()-9,item.length()-4);
			praytime += t + ',';
			//Serial.println(t);
		}

		return praytime;
	}
	else
	{
		Serial.println("Wifi connection failed");
		return "0";
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

//=== Setup&Loop ----------------------------------------
void setup() {
	Serial.begin(9600);
	pinMode(CONFIGRESETBUTTON, INPUT_PULLUP);
	Serial.println("boot");
  mx.begin(); // init led matrix
	
  setupWiFi(); // setup wifif
  Serial.println("Wifi connected.");

  NTPclient.begin(ntpserver, CET);
  setSyncInterval(SECS_PER_HOUR);
  setSyncProvider(getNTPtime);

  getLamad();  // get lamad time 
  
  mp3.begin();
  mp3.setSynchronous(true);
  mp3.volume(mp3.volumeMax());
}
void loop() {
  mp3.check(); 
  uint8_t	timeslice=now() % 60;  
  int now = getMinute(getNow()); 
  if (now == 0 ) {
    NTPclient.begin(ntpserver, CET);
    setSyncInterval(SECS_PER_HOUR);
    setSyncProvider(getNTPtime);
  }
 
	if ( (timeslice > 4 && timeslice < 9) ||  (timeslice > 24 && timeslice < 28) ) {
    showNextPray();
	}
	else {
		display_clock();
	}

  if ( now == getMinute(p_time[0]) && prevPlaying < now) {
    Serial.println("Play 1");
    prevPlaying = now;
    mp3.playTrack(1);
  }
  else if ((now  == getMinute(p_time[1]) || now  == getMinute(p_time[2]) || now  == getMinute(p_time[3]) || now  == getMinute(p_time[4])) && prevPlaying < now) {
    Serial.println("Play 2");
    prevPlaying = now;
    mp3.playTrack(2);
  }
  else if (now == getMinute("01:00") && prevPlaying < now) {
    Serial.println("get prayer time");
    getLamad();
    prevPlaying = now;
  }

	if (digitalRead(CONFIGRESETBUTTON)==LOW) { // & false to skip resetWiFi
		if (reset_trigger == false) {
			reset_trigger = true;
			Serial.println("Config reset button pressed, taking action");
			resetWiFi(); 
		}
	}
}
