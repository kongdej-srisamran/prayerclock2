#include "lamad.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "StringSplitter.h"

const char *host = "http://www.muslimthaipost.com/prayertimes/solaat.php?jAcHwyO2RBVVRPO21BVVRPO3lBVVRPOzEzMDsxNjY7IzAwMDBGRjsjMDAwMEZGOyNGRkZGRkY7I0ZGRkZGRjsjRkZGRkZGOyNGRkZGRkY7I0ZGRkZGRjsjMDAwMDAwOyMwMDAwMDA7IzAwMDAwMDs7Ozs7Ozs7Ozs7Ozs7Ozs7MjswOzA7cGR8UFM7MTs7MTsxLjI7YzYz";


String getLamadData()
{
	String praytime;
	String url = String(host);
	String line;
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
