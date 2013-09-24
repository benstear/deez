#ifndef SAMFile_H
#define SAMFile_H

#include "Common.h"
#include "Parsers/SAMParser.h"
#include "Fields/Sequence.h"
#include "Fields/ReadName.h"
#include "Fields/MappingFlag.h"
#include "Fields/MappingLocation.h"
#include "Fields/MappingQuality.h"
#include "Fields/QualityScore.h"
#include "Fields/PairedEnd.h"
#include "Fields/OptionalField.h"

class SAMFileCompressor {
	SAMParser parser;

	SequenceCompressor		   reference;
	ReadNameCompressor 		   readName;
	MappingFlagCompressor 	   mappingFlag;
	MappingLocationCompressor  mappingLocation;
    MappingQualityCompressor   mappingQuality;
	QualityScoreCompressor 	   queryQual;
	PairedEndCompressor 	   pairedEnd;
	OptionalFieldCompressor    optionalField;

	FILE *outputFile;
	FILE *indexFile;

	int  blockSize;

public:
	SAMFileCompressor (const std::string &outFile, const std::string &samFile, const std::string &genomeFile, int blockSize);
	~SAMFileCompressor (void);

private:
	void outputBlock (Compressor *c, size_t k);

public:
	void compress (void);
};

class SAMFileDecompressor {
	SequenceDecompressor   			reference;
	ReadNameDecompressor 			readName;
	MappingFlagDecompressor 		mappingFlag;
	MappingLocationDecompressor 	mappingLocation;
	MappingQualityDecompressor		mappingQuality;
	EditOperationDecompressor 		editOperation;
	QualityScoreDecompressor 		queryQual;
	PairedEndDecompressor 			pairedEnd;
	OptionalFieldDecompressor       optionalField;

	FILE *samFile;
	FILE *inFile;
	FILE *idxFile;

    int blockSize;

public:
	SAMFileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~SAMFileDecompressor (void);

private:
	bool getSingleBlock (Array<uint8_t> &in);
	int getBlock (std::string &);

public:
	void decompress (void);
	void decompress (const string &range);
};

#endif
