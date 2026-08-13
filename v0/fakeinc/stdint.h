#ifndef FAKEINC_STDINT_H
#define FAKEINC_STDINT_H
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
#define INT32_MIN (-2147483647-1)
#define INT32_MAX 2147483647
#define INT64_MIN (-9223372036854775807-1)
#define INT64_MAX 9223372036854775807
typedef int bool;
#define true 1
#define false 0
#endif
