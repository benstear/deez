#ifndef BAMParser_H
#define BAMParser_H

#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

#include "../Common.h"
#include "Parser.h"
#include "Record.h"

class BAMParser: public Parser {
	gzFile input;
	FILE *fd;
	shared_ptr<File> webFile;

	Record currentRecord;
    size_t file_size;

    int32_t chromosomesCount;
    char **chromosomes;

    int32_t dataSize;
    char *data;

public:
	BAMParser (const std::string &filename);
	~BAMParser (void);

private:
	void readChromosomeInformation(void);

public:
	std::string readComment (void);
	bool readNext (void);
	//bool readRaw(Array<uint8_t> &a);
	bool hasNext (void);
	size_t fpos (void);
	size_t fsize (void);

public:
	void parse (void);
	Record next (void);
	std::string head (void);
};

#endif
