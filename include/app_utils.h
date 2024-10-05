#ifndef __APP_UTILS_H
#define __APP_UTILS_H

#include <stdint.h>


extern void app_error_handler(uint32_t error_code, uint32_t line_number,
		const uint8_t* filename);

#define ERROR_CHECK(__error_code)                        \
	do                                                   \
	{                                                    \
		const uint32_t __local_code = (__error_code);    \
		if (__local_code != 0)                 \
		{                                                \
			app_error_handler(__local_code, __LINE__,    \
					(const uint8_t *) __FILE__);         \
		}                                                \
	} while (0)

#define ASSERT_CHECK(__cond)                             \
	do                                                   \
	{                                                    \
		const uint32_t __local_cond = (__cond);          \
		if (!__local_cond)                               \
		{                                                \
			app_error_handler(0xFFFF, __LINE__,          \
					(const uint8_t *) __FILE__);         \
		}                                                \
	} while (0)

#define RETURN_ON_ERROR(status)    \
	do {                           \
		uint32_t result = status;  \
		if (result != 0) \
		{                          \
			return result;         \
		}                          \
	} while (0);



#endif /* __APP_UTILS_H */