#include "Arduino.h"
#include "main.h"
#include "app_utils.h"

#include <TinyGsmClient.h>
#include "LittleFS.h"

#include "thingcell_boardv1.h"

//TODO use webinterface and littleFS to configure
#include "credentials.h"

/* Global variables ***********************************************************/

static TinyGsm modem(SerialAT);
TinyGsmClientSecure client(modem); //extern

/* Function prototypes ********************************************************/

static int sim_store_file(const char* filename, const char* store_name);


/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/

static int sim_store_file(const char* filename, const char* store_name)
{
	
	File file = LittleFS.open(filename);
	if(!file || file.isDirectory()){
		Serial.println("- failed to open file for reading");
		return 10;
	}

	String content = file.readString();
	file.close();

	char buff[100];
	snprintf(buff, 100, "+FSCREATE=%s", store_name);
	modem.sendAT(buff);
	if (modem.waitResponse() != 1) {
		return 20;
	}

	const int file_size = content.length();

	snprintf(buff, 100, "+FSWRITE=%s,0,%d,10", store_name, file_size);
	modem.sendAT(buff);
	if (modem.waitResponse(GF(">")) != 1) {
		return 30;
	}

	for (int i = 0; i < file_size; i++) {
		char c = content.charAt(i);
		modem.stream.write(c);
	}

	modem.stream.write(AT_NL);
	modem.stream.flush();

	if (modem.waitResponse(2000) != 1) {
		return 40;
	}

	return 0;
}

/* Main functions *************************************************************/
int get_gps_data(float* lat, float* lon, float* accuracy)
{
	float speed, alt;
	int vsat, usat, year, month, day, hour, min, sec;
	SerialMon.println("Requesting GPS");
	int ret = modem.getGPS(lat, lon, &speed, &alt, &vsat, &usat, accuracy,
			&year, &month, &day, &hour, &min, &sec);
	if (!ret) {
		return 0;
	}

	SerialMon.print("Latitude: ");
	SerialMon.print(String(*lat, 8));
	SerialMon.print(" longitude: ");
	SerialMon.println(String(*lon, 8));
	SerialMon.print("Speed: ");
	SerialMon.print(speed);
	SerialMon.print(", altitude: ");
	SerialMon.println(alt);
	SerialMon.print("Visible Satellites: ");
	SerialMon.print(vsat);
	SerialMon.print(" (");
	SerialMon.print(usat);
	SerialMon.println(" used)");
	SerialMon.print("Accuracy: ");
	SerialMon.println(*accuracy);
	SerialMon.print(day);
	SerialMon.print("/");
	SerialMon.print(month);
	SerialMon.print("/");
	SerialMon.print(year);
	SerialMon.print(" ");
	SerialMon.print(hour);
	SerialMon.print(":");
	SerialMon.print(min);
	SerialMon.print(":");
	SerialMon.println(sec);

	modem.disableGPS();

	return 1;
}

int sim_setup()
{
	
	digitalWrite(PIN_SIM_EN, HIGH);
	delay(100);

	pinMode(PIN_PWRKEY, OUTPUT);
	digitalWrite(PIN_PWRKEY, LOW);
	delay(1400);
	digitalWrite(PIN_PWRKEY, HIGH);
	delay(6000);

	SerialAT.begin(115200, SERIAL_8N1, PIN_AT_RX, PIN_AT_TX);

	SerialMon.println("Initializing modem...");
	modem.restart();
	delay(20000);
	String modemInfo = modem.getModemInfo();
	if (modemInfo.length() == 0) {
		return 1;
	}

	SerialMon.print("Modem Info: ");
	SerialMon.println(modemInfo);

	// Unlock your SIM card with a PIN if needed
	if (SIM_PIN && modem.getSimStatus() != 3) {
		modem.simUnlock(SIM_PIN);
	}

	SerialMon.print("Sim status: ");
	SerialMon.println(modem.getSimStatus());

	SerialMon.println("Enabling GPS...");
	modem.enableGPS();
	delay(1000);

	SerialMon.println("Modem initialized!");

	return 0;
}

int sim_connect()
{

	SerialMon.print("Waiting for network... ");
	if (!modem.waitForNetwork()) {
		SerialMon.println("Fail");
		return 1;
	}
	SerialMon.println("Success");

	if (modem.isNetworkConnected()) {
		SerialMon.println("Network connected");
	}

	SerialMon.printf("Connecting to %s ", GPRS_APN);
	if (!modem.gprsConnect(GPRS_APN, GPRSUSER, GPRSPASS)) {
		SerialMon.println("Fail");
		return 2;
	}
	SerialMon.println("Success");

	if (modem.isGprsConnected()) {
		SerialMon.println("GPRS connected");
	}

	return 0;
}

int sim_upload_ssl(const char* filename)
{

	SerialMon.print("Upload ca.crt file ");
	//TODO check if file exist
	RETURN_ON_ERROR(sim_store_file(filename, "ca.crt"));

	modem.sendAT("+SSLSETCERT=\"" "ca.crt" "\"");
	if (modem.waitResponse() != 1) {
		SerialMon.println("Fail");
		return 3;
	}

	if (modem.waitResponse(5000L, AT_NL "+SSLSETCERT:") != 1) {
		SerialMon.println("Fail");
		return 4;
	}

	return modem.stream.readStringUntil('\n').toInt();
}

void sim_poweroff()
{
	SerialMon.println("Powering off modem");


	// modem.gprsDisconnect();
	// SerialMon.println(F("GPRS disconnected"));

	modem.poweroff();
	delay(1000);

	digitalWrite(PIN_SIM_EN, LOW);

}
