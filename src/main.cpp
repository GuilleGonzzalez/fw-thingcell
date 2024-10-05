#include "Arduino.h"
#include "main.h"
#include "app_utils.h"

#include <PubSubClient.h>
#include "LittleFS.h"

#include "thingcell_boardv1.h"

//TODO use webinterface and littleFS to configure
#include "credentials.h"


/* Global variables ***********************************************************/

static const char* topic_init       = "init";
static const char* topic_led        = "led";
static const char* topic_gps        = "gps";
static const char* topic_led_status = "ledStatus";
static const char* topic_gps_status = "gpsStatus";

static bool led_status = 0;
static uint32_t last_time = 0;

static PubSubClient mqtt(client);

/* Function prototypes ********************************************************/

static void mqtt_cb(char* topic, byte* payload, unsigned int len);
static int mqtt_connect();

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

static int mqtt_connect() {
	SerialMon.print("Connecting to ");
	SerialMon.print(MQTT_HOST);
	bool status = mqtt.connect("ThingCellTest", MQTT_USER, MQTT_PASSWORD);
	if (status == false) {
		SerialMon.println(" Fail");
		return 1;
	}
	SerialMon.println(" Success");
	mqtt.publish(topic_init, "ThingCellTest started");
	mqtt.subscribe(topic_led);
	mqtt.subscribe(topic_gps);
	if(mqtt.connected()) {
		return 0;
	} else {
		return 2;
	}
}
/* Setup **********************************************************************/

void setup()
{
	SerialMon.begin(115200);

	delay(10);

	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_SIM_EN, OUTPUT);

	ASSERT_CHECK(LittleFS.begin(true));

	SerialMon.println("LittleFS mounted successfully");

	ERROR_CHECK(sim_setup());
	ERROR_CHECK(sim_connect());
	ERROR_CHECK(sim_upload_ssl("/ca.crt"));

	SerialMon.println("MQTT init...");
	mqtt.setServer(MQTT_HOST, MQTT_PORT);
	mqtt.setCallback(mqtt_cb);

	// if(client.connected()) {
	// 	Serial.println("GSM client available");
	// }

	//uint32_t timeout = millis();
	// while (client.connected() && millis() - timeout < 10000L) {
	// 	while (client.available()) {
	// 		char c = client.read();
	// 		SerialMon.print(c);
	// 		timeout = millis();
	// 	}
	// }
	last_time = millis();
}

/* Loop **********************************************************************/

void loop()
{
	// if (mqtt.connected()) {
	// 	mqtt.loop();
	// } else {
	// 	SerialMon.println("MQTT not connected");
	// 	mqtt_connect();
	// 	delay(1000);
	// }
	if (millis() - last_time > 10000) {
		sim_poweroff();
		// esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
		esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
		Serial.println("Configured all RTC Peripherals to be powered down in sleep");
		Serial.println("Going to sleep now");
		delay(1000);
		Serial.flush(); 
		esp_deep_sleep_start();
		Serial.println("This will never be printed");
	}
}