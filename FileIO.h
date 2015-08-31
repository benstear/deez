#ifndef FileIO_H
#define FileIO_H

#include "Common.h"
#include <curl/curl.h>
#include <zlib.h>

using namespace std;

class File
{
	FILE *fh;
	size_t fsize;

protected:
	File ();

public:
	File (const char *path, const char *mode);
	virtual ~File ();

	virtual void open (const char *path, const char *mode);
	virtual void close ();

	virtual ssize_t read (void *buffer, size_t size);
	virtual ssize_t read(void *buffer, size_t size, size_t offset);
	virtual ssize_t write (void *buffer, size_t size);

	virtual uint8_t readU8();
	virtual uint16_t readU16();
	virtual uint32_t readU32();
	virtual uint64_t readU64();

	virtual ssize_t tell ();
	virtual ssize_t seek (size_t pos);

	virtual size_t size ();
	virtual bool eof ();

private:
	virtual void get_size ();
};

class WebFile: public File 
{
	CURL *ch;
	size_t fsize, foffset;

public:
	WebFile (const char *path, const char *mode);
	~WebFile ();

	void open (const char *path, const char *mode);
	void close ();

	ssize_t read (void *buffer, size_t size);
	ssize_t read(void *buffer, size_t size, size_t offset);
	ssize_t write (void *buffer, size_t size);

	ssize_t tell ();
	ssize_t seek (size_t pos);

	size_t size ();
	bool eof ();

private:
	void get_size ();

	struct CURLBuffer {
		char *data;
		size_t size;
		CURLBuffer () : data(0), size(0) {};
		~CURLBuffer () { free(data); }
	};
	static size_t CURLCallback (void *ptr, size_t size, size_t nmemb, void *data);
};

class GzFile: public File
{
	gzFile fh;

public:
	GzFile (const char *path, const char *mode);
	~GzFile ();

	void open (const char *path, const char *mode);
	void close ();

	ssize_t read (void *buffer, size_t size);
	ssize_t read(void *buffer, size_t size, size_t offset);
	ssize_t write (void *buffer, size_t size);

	ssize_t tell ();
	ssize_t seek (size_t pos);

	size_t size ();
	bool eof ();

private:
	void get_size ();
};

File *OpenFile (const char *path, const char *mode);

#endif // FileIO_H
