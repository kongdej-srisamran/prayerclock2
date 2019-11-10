#ifndef WIFI_H
#define	WIFI_H

#define NTP_LEN		40
#define APITOKEN_LEN	255

extern char ntpserver[NTP_LEN];
extern char apitoken[APITOKEN_LEN];

void setupWiFi(void);
void resetWiFi(void);

#endif /* WIFI_H */
