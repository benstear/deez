#include <thread>
#include <utility>
#include "Sequence.h"
using namespace std;

SequenceCompressor::SequenceCompressor (const string &refFile, int bs):
	reference(refFile), 
	fixes_loc(MB, MB),
	fixes_replace(MB, MB),
	fixed(0),
	chromosome("") // first call to scanNextChr will populate this one
{
	fixesStream = new GzipCompressionStream<6>();
	fixesReplaceStream = new GzipCompressionStream<6>();
}

SequenceCompressor::~SequenceCompressor (void) 
{
	delete[] fixed;
	delete fixesStream;
	delete fixesReplaceStream;
}

void SequenceCompressor::updateBoundary (size_t loc) 
{
	maxEnd = max(maxEnd, loc);
}

void SequenceCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (chromosome == "*") 
		return;

	out.add((uint8_t*)&fixedStart, sizeof(size_t));
	out.add((uint8_t*)&fixedEnd,   sizeof(size_t));
	out_offset += sizeof(size_t) * 2;
	compressArray(fixesStream, fixes_loc, out, out_offset);
	compressArray(fixesReplaceStream, fixes_replace, out, out_offset);

	fixes_loc.resize(0);
	fixes_replace.resize(0);
}

void SequenceCompressor::scanChromosome (const string &s) 
{
	// by here, all should be fixed ...
	assert(fixes_loc.size() == 0);
	assert(fixes_replace.size() == 0);
	
	// clean genomePager
	delete[] fixed;
	fixed = 0;
	fixedStart = fixedEnd = maxEnd = 0;

	chromosome = reference.scanChromosome(s);
}

// called at the end of the block!
// IS NOT ATOMIC!
inline void SequenceCompressor::updateGenomeLoc (size_t loc, char ch, Array<int*> &stats) 
{
	assert(loc < stats.size());
	if (!stats.data()[loc]) {
		stats.data()[loc] = new int[6];
		memset(stats.data()[loc], 0, sizeof(int) * 6);
	}
	stats.data()[loc][getDNAValue(ch)]++;
}

void SequenceCompressor::applyFixesThread(EditOperationCompressor &editOperation, Array<int*> &stats, size_t fixedStart, size_t offset, size_t size) 
{
	ZAMAN_START();
	for (size_t k = 0; k < editOperation.size(); k++) {
		if (editOperation[k].op == "*" || editOperation[k].seq == "*") 
			continue;
		if (editOperation[k].start >= offset + size)
			break;
		if (editOperation[k].end <= offset)
			continue;
		
		size_t genPos = editOperation[k].start;
		size_t num = 0;
		size_t seqPos = 0;
		for (size_t pos = 0; genPos < offset + size && pos < editOperation[k].op.length(); pos++) {
			if (isdigit(editOperation[k].op[pos])) {
				num = num * 10 + (editOperation[k].op[pos] - '0');
				continue;
			}
			switch (editOperation[k].op[pos]) {
				case 'M':
				case '=':
				case 'X':
					for (size_t i = 0; genPos < offset + size && i < num; i++, genPos++, seqPos++)
						if (genPos >= offset)
							updateGenomeLoc(genPos - fixedStart, editOperation[k].seq[seqPos], stats);
					break;
				case 'D':
				case 'N':
					for (size_t i = 0; genPos < offset + size && i < num; i++, genPos++)
						if (genPos>= offset)
							updateGenomeLoc(genPos - fixedStart, '.', stats);
					break;
				case 'I':
				case 'S':
					seqPos += num;
					break;
			}
			num = 0;
		}
	}
	ZAMAN_END("T~");
}

/*
 * in  nextBlockBegin: we retrieved all reads starting before nextBlockBegin
 * out start_S
 */
size_t SequenceCompressor::applyFixes (size_t nextBlockBegin, EditOperationCompressor &editOperation,
	size_t &start_S, size_t &end_S, size_t &end_E, size_t &fS, size_t &fE) 
{
	if (editOperation.size() == 0) 
		return 0;

	if (chromosome != "*") {
		// Previously, we fixed all locations
		// from fixedStart to fixedEnd
		// New reads locations may overlap fixed locations
		// so fixingStart indicates where should we start
		// actual fixing

		// If we can fix anything
		size_t fixingStart = fixedEnd;
		ZAMAN_START();
		// Do we have new region to fix?

		if (maxEnd > fixedEnd) {
			// Determine new fixing boundaries
			size_t newFixedEnd = maxEnd;
			size_t newFixedStart = editOperation[0].start;
			assert(fixedStart <= newFixedStart);

			// Update fixing table
			char *newFixed = new char[newFixedEnd - newFixedStart];
			// copy old fixes!
			if (fixed && newFixedStart < fixedEnd) {
				memcpy(newFixed, fixed + (newFixedStart - fixedStart), 
					fixedEnd - newFixedStart);
				reference.load(newFixed + (fixedEnd - newFixedStart), 
					fixedEnd, newFixedEnd);
			}
			else reference.load(newFixed, newFixedStart, newFixedEnd);

			fixedStart = newFixedStart;
			fixedEnd = newFixedEnd;
			delete[] fixed;
			fixed = newFixed;
		}
		
		ZAMAN_END("S1");

		//	SCREEN("Given boundary is %'lu\n", nextBlockBegin);
		//	SCREEN("Fixing from %'lu to %'lu\n", fixedStart, fixedEnd);
		//	SCREEN("Reads from %'lu to %'lu\n", records[0].start, records[records.size()-1].end);

		// obtain statistics

		Array<int*> stats(0, MB); 
		ZAMAN_START();
		stats.resize(fixedEnd - fixedStart);
		memset(stats.data(), 0, stats.size() * sizeof(int*));

		vector<std::thread> t;
		size_t sz = stats.size() / optThreads + 1;
		for (int i = 0; i < optThreads; i++)
			t.push_back(thread(applyFixesThread, 
				ref(editOperation), ref(stats), fixedStart, 
				fixedStart + i * sz, min(sz, stats.size() - i * sz)
			));
		for (int i = 0; i < optThreads; i++)
			t[i].join();
		ZAMAN_END("S2"); 

		// patch reference genome
		size_t fixedPrev = 0;
		ZAMAN_START();
		fixes_loc.resize(0);
		fixes_replace.resize(0);
		for (size_t i = 0; i < fixedEnd - fixedStart; i++) if (stats.data()[i]) {
			int max = -1, pos = -1;
			int max2 = -1, pos2 = -1;
			int sum = 0; ///stats.data()[i][0];
			for (int j = 1; j < 6; j++) {
				sum += stats.data()[i][j];
				if (stats.data()[i][j] > max)
					max2 = max, pos2 = pos, max = stats.data()[i][pos = j];
				else if (stats.data()[i][j] > max2)
					max2 = stats.data()[i][pos2 = j];
			}
			if (fixed[i] != pos[".ACGTN"] || max2 / double(sum) > 0.2) 
			{
				// +1 for 0 termninator avoid
				addEncoded(fixedStart + i - fixedPrev + 1, fixes_loc);
				fixedPrev = fixedStart + i;
				
				if (max2 / double(sum) > 0.2) { // allelic test
					char c = 85 + pos * 6 + pos2;
					fixes_replace.add(fixed[i] = c);
				}
				else 
					fixes_replace.add(fixed[i] = pos[".ACGTN"]);
			}
			//printf("%d %d %d %d %d %d %.2lf %c\n", fixedStart+i,max,pos,max2,pos2,sum,max2/double(sum), fixed[i]);
			delete[] stats.data()[i];
		}
		ZAMAN_END("S3");
	}

	// generate new cigars
	size_t bound;
	for (bound = 0; bound < editOperation.size() && editOperation[bound].end <= fixedEnd; bound++);
	start_S = editOperation[0].start;
	end_S = editOperation[bound-1].start;
	end_E = editOperation[bound-1].end;
	fS = fixedStart;
	fE = fixedEnd;
	editOperation.setFixed(fixed, fixedStart);

	return bound;
}

/************************************************************************************************************************/

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	chromosome(""),
	fixed(0)
{
	fixesStream = new GzipDecompressionStream();
	fixesReplaceStream = new GzipDecompressionStream();
}

SequenceDecompressor::~SequenceDecompressor (void)
{
	delete[] fixed;
	delete fixesStream;
	delete fixesReplaceStream;
}


bool SequenceDecompressor::hasRecord (void) 
{
	return true;
}

void SequenceDecompressor::importRecords (uint8_t *in, size_t in_size)  
{
	if (chromosome == "*" || in_size == 0)
		return;

	size_t newFixedStart = *(size_t*)in; in += sizeof(size_t);
	size_t newFixedEnd   = *(size_t*)in; in += sizeof(size_t);
	
	Array<uint8_t> fixes_loc;
	decompressArray(fixesStream, in, fixes_loc);
	Array<uint8_t> fixes_replace;
	decompressArray(fixesReplaceStream, in, fixes_replace);

	////////////
	// NEED FROM READSSTART
	assert(fixedStart <= newFixedStart);	
	char *newFixed = new char[newFixedEnd - newFixedStart];
	// copy old fixes!
	if (fixed && newFixedStart < fixedEnd) {
		memcpy(newFixed, fixed + (newFixedStart - fixedStart), 
			fixedEnd - newFixedStart);
		reference.load(newFixed + (fixedEnd - newFixedStart), 
			fixedEnd, newFixedEnd);
	}
	else reference.load(newFixed, newFixedStart, newFixedEnd);

	fixedStart = newFixedStart;
	fixedEnd = newFixedEnd;
	delete[] fixed;
	fixed = newFixed;
	
	size_t prevFix = 0;
	uint8_t *len = fixes_loc.data();
	for (size_t i = 0; i < fixes_replace.size(); i++) {
		prevFix += getEncoded(len) - 1;
		assert(prevFix < fixedEnd);
		assert(prevFix >= fixedStart);
		fixed[prevFix - fixedStart] = fixes_replace.data()[i];

		changes[prevFix] = fixes_replace.data()[i];
	}
}

void SequenceDecompressor::setFixed (EditOperationDecompressor &editOperation) 
{
	editOperation.setFixed(fixed, fixedStart);
}

void SequenceDecompressor::scanChromosome (const string &s)
{
	// by here, all should be fixed ...
	// 	TODO more checking
	//	assert(fixes.size() == 0);
	//	assert(records.size() == 0);
	//	assert(editOperation.size() == 0);

	// clean genomePager
	delete[] fixed;
	fixed = 0;
	fixedStart = fixedEnd = 0;
	chromosome = reference.scanChromosome(s);
}
