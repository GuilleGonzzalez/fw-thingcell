#include "Arduino.h"
#define TINY_GSM_MODEM_SIM868
//TODO #define TINY_GSM_SSL_CLIENT_AUTHENTICATION 
//TODO #include <SSLClient.h>

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include "LittleFS.h"

//TODO use webinterface and littleFS to configure
#include "credentials.h"

#define PIN_PWRKEY 34
#define PIN_AT_RX 14
#define PIN_AT_TX 13

#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon

#define GSM_PIN ""
#define PIN_LED 4


/* Global variables ***********************************************************/

static const char* apn      = "tm";
static const char* gprsUser = "";
static const char* gprsPass = "";


static const char* topic_init       = "init";
static const char* topic_led        = "led";
static const char* topic_gps        = "gps";
static const char* topic_led_status = "ledStatus";
static const char* topic_gps_status = "gpsStatus";

static bool led_status = 0;
static uint32_t last_time = 0;

TinyGsm modem(SerialAT);
TinyGsmClientSecure client(modem);
PubSubClient mqtt(client);

/* Function prototypes ********************************************************/

static void mqtt_cb(char* topic, byte* payload, unsigned int len);
static bool mqtt_connect();
static int get_gps_data(float* lat, float* lon, float* accuracy);
static int sim_store_file(const char* filename, const char* store_name);


/* Callbacks ******************************************************************/

static void mqtt_cb(char* topic, byte* payload, unsigned int len) {
	SerialMon.print("MQTT received [");
	SerialMon.print(topic);
	SerialMon.print("]: ");
	SerialMon.write(payload, len);
	SerialMon.println();

	if (String(topic) == topic_led) {
		led_status = !led_status;
		digitalWrite(PIN_LED, led_status);
		mqtt.publish(topic_led_status, led_status ? "1" : "0");
	} else if (String(topic) == topic_gps) {
		float lat, lon, acc;
		int rc = get_gps_data(&lat, &lon, &acc);
		if (rc) {
			char str[100];
			sprintf(str, "{\"latitude\": %f, \"longitude\": %f, \"gps_accuracy\": %f}",
					lat, lon, acc);
			SerialMon.println(str);
			mqtt.publish(topic_gps_status, str);
		} else {
			SerialMon.println("No GPS data");
		}
	}
}

/* Function definitions *******************************************************/

static bool mqtt_connect() {
	SerialMon.print("Connecting to ");
	SerialMon.print(MQTT_HOST);
	boolean status = mqtt.connect("ThingCellTest", MQTT_USER, MQTT_PASSWORD);
	if (status == false) {
		SerialMon.println(" Fail");
		return false;
	}
	SerialMon.println(" Success");
	mqtt.publish(topic_init, "ThingCellTest started");
	mqtt.subscribe(topic_led);
	mqtt.subscribe(topic_gps);
	return mqtt.connected();
}

static int get_gps_data(float* lat, float* lon, float* accuracy)
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

static int sim_store_file(const char* filename, const char* store_name)
{
	
	File file = LittleFS.open(filename);
	if(!file || file.isDirectory()){
		Serial.println("- failed to open file for reading");
		return 1;
	}

	String content = file.readString();
	file.close();

	char buff[100];
	snprintf(buff, 100, "+FSCREATE=%s", store_name);
	modem.sendAT(buff);
	if (modem.waitResponse() != 1) {
		return 1;
	}

	const int file_size = content.length();

	snprintf(buff, 100, "+FSWRITE=%s,0,%d,10", store_name, file_size);
	modem.sendAT(buff);
	if (modem.waitResponse(GF(">")) != 1) {
		return 1;
	}

	for (int i = 0; i < file_size; i++) {
		char c = content.charAt(i);
		modem.stream.write(c);
	}

	modem.stream.write(AT_NL);
	modem.stream.flush();

	if (modem.waitResponse(2000) != 1) {
		return 1;
	}

	return 0;
}

/* Setup **********************************************************************/

void setup()
{
	SerialMon.begin(115200);

	delay(10);

	pinMode(PIN_LED, OUTPUT);
	digitalWrite(PIN_LED, LOW);

	pinMode(PIN_PWRKEY, OUTPUT);
	digitalWrite(PIN_PWRKEY, LOW);
	delay(1500);
	digitalWrite(PIN_PWRKEY, HIGH);
	delay(6000);


	if (!LittleFS.begin(true)) {
		SerialMon.println("An error has occurred while mounting LittleFS");
		delay(10000);
		return;
	}

	SerialMon.println("LittleFS mounted successfully");

	SerialMon.printf("Reading file: %s\r\n", "/ca.crt");

	File file = LittleFS.open("/ca.crt");
	if(!file || file.isDirectory()){
		Serial.println("- failed to open file for reading");
	} else {
		while(file.available()) {
			String line = file.readStringUntil('\n');
			SerialMon.println(line);
		}
	}


	file.close();

	SerialAT.begin(115200, SERIAL_8N1, PIN_AT_RX, PIN_AT_TX);

	SerialMon.println("Initializing modem...");
	modem.restart();
	delay(20000);
	String modemInfo = modem.getModemInfo();
	SerialMon.print("Modem Info: ");
	SerialMon.println(modemInfo);

	// Unlock your SIM card with a PIN if needed
	if (GSM_PIN && modem.getSimStatus() != 3) {
		modem.simUnlock(GSM_PIN);
	}

	SerialMon.print("Sim status: ");
	SerialMon.println(modem.getSimStatus());

	SerialMon.println("Enabling GPS...");
	modem.enableGPS();
	delay(5000);


	SerialMon.println("System initialized!");

	SerialMon.print("Waiting for network... ");
	if (!modem.waitForNetwork()) {
		SerialMon.println("Fail");
		delay(10000);
		return;
	}
	SerialMon.println("Success");

	if (modem.isNetworkConnected()) {
		SerialMon.println("Network connected");
	}

	SerialMon.printf("Connecting to %s ", apn);
	if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
		SerialMon.println("Fail");
		delay(10000);
		return;
	}
	SerialMon.println("Success");

	if (modem.isGprsConnected()) {
		SerialMon.println("GPRS connected");
	}

	SerialMon.print("Upload ca.crt file ");
	//TODO check if file exist
	int ret = sim_store_file("/ca.crt", "ca.crt");

	if (ret != 0) {
		SerialMon.println("Fail");
		delay(10000);
		return;
	}

	modem.sendAT("+SSLSETCERT=\"" "ca.crt" "\"");
	if (modem.waitResponse() != 1) {
		SerialMon.println("Fail");
		delay(10000);
		return;
	}

	if (modem.waitResponse(5000L, AT_NL "+SSLSETCERT:") != 1) {
		SerialMon.println("Fail");
		delay(10000);
		return;
	}

	ret = modem.stream.readStringUntil('\n').toInt();

	if (ret != 0) {
		SerialMon.println("Fail");
		delay(10000);
		return;
	}

	SerialMon.println("Success");

	SerialMon.println("MQTT init...");
	mqtt.setServer(MQTT_HOST, MQTT_PORT);
	mqtt.setCallback(mqtt_cb);


	uint32_t timeout = millis();
	while (client.connected() && millis() - timeout < 10000L) {
		while (client.available()) {
			char c = client.read();
			SerialMon.print(c);
			timeout = millis();
		}
	}
	SerialMon.println();

	// modem.gprsDisconnect();
	// SerialMon.println(F("GPRS disconnected"));
}

/* Loop **********************************************************************/

void loop()
{
	if (!mqtt.connected()) {
		SerialMon.println("MQTT not connected");
		uint32_t t = millis();
		if (t - last_time > 10000) {
			last_time = t;
			if (mqtt_connect()) {
				last_time = 0;
			}
		}
		delay(100);
		return;
	}

	mqtt.loop();
}