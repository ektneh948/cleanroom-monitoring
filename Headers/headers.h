#ifndef __HEADERS_H__
#define __HEADERS_H__

#define DEBUG 1

#define SQL_CLN_NAME	"CMS_SQL"

#define BUFFER_MAX_COUNT 128
#define COMMAND_MAX_COUNT 8

typedef enum
{
	E_MONITORING,
	E_MANUAL,
} E_MODE;

typedef struct
{
	float ptcl;
	float temp;
	float humi;
} SENSOR_DATA;

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

static inline float median3_float(float a, float b, float c) {
    if (a > b) { float t=a; a=b; b=t; }
    if (b > c) { float t=b; b=c; c=t; }
    if (a > b) { float t=a; a=b; b=t; }
    return b;
}

#endif //__HEADERS_H__
