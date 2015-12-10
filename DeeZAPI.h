#ifndef DeeZAPI_H
#define DeeZAPI_H

#include "Common.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Parsers/Record.h"
#include "Sort.h"
#include "Common.h"
#include "Decompress.h"

class DeeZFile : public FileDecompressor {
public:
	struct SAMRecord {
		string rname;
		int flag;
		string chr;
		unsigned long loc;
		int mapqual;
		string cigar;
		string pchr;
		unsigned long ploc;
		int tlen;
		string seq;
		string qual;
		string opt;
	};

private:
	std::vector<SAMRecord> records;

protected:
    virtual inline void printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual,
        const string &qual, const string &optional, const PairedEndInfo &pe, int file)
    {
    	records.push_back({rname, flag, chr, eo.start, mqual, eo.op, pe.chr, pe.pos, pe.tlen, eo.seq, qual, optional});
    }
    virtual inline void printComment(int file) {
    }

public:
	DeeZFile (const std::string &inFile, const std::string &genomeFile = ""):
		FileDecompressor(inFile, "", genomeFile, optBlock) 
	{
	}
	~DeeZFile (void) {}

public:
	void setLogLevel (int level) {
		if (level < 0 || level > 2)
			throw DZException("Log level must be 0, 1 or 2");
		optLogLevel = level;
	}

	int getFileCount () {
		return comments.size();
	}

	std::string getComment (int file) {
		if (file < 0 || file >= comments.size())
			throw DZException("Invalid file index");
		return comments[file];
	}

	std::vector<SAMRecord> &getRecords (const std::string &range, int filterFlag = 0) {
		records.clear();
		FileDecompressor::decompress(range, filterFlag);
		return records;
	}
};

#endif // DeeZAPI_H