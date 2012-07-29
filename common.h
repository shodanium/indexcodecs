#pragma once
#define _SECURE_SCL 0
#pragma runtime_checks("",off)
#define _HAS_ITERATOR_DEBUGGING 0

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#include <stdarg.h>
#include <map>
#include <math.h>

using namespace std;

typedef unsigned char ui8;
typedef unsigned short ui16;
typedef unsigned int ui32;
typedef unsigned long long ui64;
typedef signed long long i64;

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

void inline die ( const char * sTemplate, ... )
{
	va_list ap;
	va_start ( ap, sTemplate );
	vprintf ( sTemplate, ap );
	va_end ( ap );
	printf ( "\n" );
	exit ( 1 );
}
