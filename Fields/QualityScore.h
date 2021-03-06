#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/SAMCompStream.h"
#include "../Engines/StringEngine.h"
#include "../Streams/ArithmeticOrder2Stream.h"
#include "../Streams/rANSOrder0Stream.h"
#include "../Streams/rANSOrder2Stream.h"

extern char optQuality;
extern char optLossy;
extern bool optNoQual;

const int QualRange = 96;

typedef 
	//ArithmeticOrder2CompressionStream<QualRange>
	rANSOrder2CompressionStream<QualRange>
	QualityCompressionStream;
typedef 
	//ArithmeticOrder2DecompressionStream<QualRange>
	rANSOrder2DecompressionStream<QualRange>
	QualityDecompressionStream;

class QualityScoreCompressor: 
	public StringCompressor<QualityCompressionStream> 
{	
	char offset;
	int stat[128];
	int lossy[128];
	bool statMode;

public:
	QualityScoreCompressor(void);

public:
	void addRecord (const std::string &qual, int flag);
	void outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k);
	static size_t shrink(char *qual, size_t len, int flag);

	int getOffset(void);
	void updateOffset(int *st);
	void calculateOffset (void);
	void offsetRecord (Record &rc);

	
private:
	static double phredScore (char c, int offset);
	void calculateLossyTable (int percentage);
	void lossyTransform (char *qual, size_t len);
};

class QualityScoreDecompressor: 
	public StringDecompressor<QualityDecompressionStream> 
{
	char offset;
	char sought;

public:
	QualityScoreDecompressor (int blockSize);

public:
	std::string getRecord (size_t i, size_t seq_len, int flag);
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);
};

#endif
