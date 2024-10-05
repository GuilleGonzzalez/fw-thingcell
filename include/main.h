#ifndef __MAIN_H
#define __MAIN_H

#define TINY_GSM_MODEM_SIM868
//TODO #define TINY_GSM_SSL_CLIENT_AUTHENTICATION 
//TODO #include <SSLClient.h>

#include <TinyGsmClient.h>

#define SerialMon Serial
#define SerialAT Serial1

extern TinyGsmClientSecure client;


int get_gps_data(float* lat, float* lon, float* accuracy);
int sim_setup();
int sim_connect();
int sim_upload_ssl(const char* filename);
void sim_poweroff();


#endif /* __MAIN_H */