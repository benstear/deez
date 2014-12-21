#ifndef Common_H
#define Common_H

#include <assert.h>
#include <inttypes.h>
#include <typeinfo>
#include <string>

#include "DZException.h"
#include "Array.h"
#include "CircularArray.h"
#include "Streams/Stream.h"

#define KB  1024LL
#define MB  KB * 1024LL
#define GB  MB * 1024LL

#define VERSION	0x10
#define MAGIC 	(0x07445A00 | VERSION)

double _zaman_ (void);
#define ZAMAN_START() {double _____zaman564265=_zaman_();
#define ZAMAN_END(s) /*DEBUG("[%s] %.2lf ", s, _zaman_()-_____zaman564265);*/}

//#define DZ_EVAL

extern bool optStdout;
extern int  optThreads;

#define LOGN(c,...)\
	fprintf(stderr, c, ##__VA_ARGS__)
#define ERROR(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define LOG(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define DEBUG(c,...)\
	//fprintf(stderr, c"\n", ##__VA_ARGS__)
#define REPEAT(x)\
	for(int _=0;_<x;_++)
#define foreach(i,c) \
	for (auto i = (c).begin(); i != (c).end(); ++i)

std::string S (const char* fmt, ...);
std::string int2str (int k);
std::string inttostr (int k);
char getDNAValue (char ch);

void addEncoded (int n, Array<uint8_t> &o);
size_t getEncoded (uint8_t *&len);

#define __debug_fwrite(a,b,c,d)
// fwrite(a,b,c,d)

extern int __DC;
extern FILE **____debug_file;
extern bool optReadLossy;

#endif
