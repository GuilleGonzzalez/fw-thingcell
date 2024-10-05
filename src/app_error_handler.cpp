#include "Arduino.h"


void app_error_handler(uint32_t error_code, uint32_t line_number,
		const uint8_t* filename)
{
	noInterrupts();

	Serial.printf("ERROR %u at %s:%u\n", error_code, filename, line_number);

	while(1);
}