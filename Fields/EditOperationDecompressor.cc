#include "EditOperation.h"
#include "Sequence.h"
using namespace std;

EditOperationDecompressor::EditOperationDecompressor (int blockSize, const SequenceDecompressor &seq):
	GenericDecompressor<EditOperation, GzipDecompressionStream>(blockSize),
	sequence(seq)
{
	streams.resize(EditOperationCompressor::Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipDecompressionStream>();
	streams[EditOperationCompressor::Fields::LOCATION] = make_shared<AC0DecompressionStream<AC, 256>>();
}

EditOperationDecompressor::~EditOperationDecompressor (void) 
{
}

void EditOperationDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(EditOperation);

	Array<uint8_t> stitches;
	decompressArray(streams[EditOperationCompressor::Fields::STITCH], in, stitches);
	Array<uint8_t> locations;
	size_t sz = decompressArray(streams[EditOperationCompressor::Fields::LOCATION], in, locations);

	ACTGStream nucleotides;
	decompressArray(streams[EditOperationCompressor::Fields::ACGT], in, nucleotides.seqvec);
	decompressArray(streams[EditOperationCompressor::Fields::ACGT], in, nucleotides.Nvec);
	nucleotides.initDecode();

	vector<Array<uint8_t>> oa(EditOperationCompressor::Fields::ACGT);
	vector<uint8_t*> fields(oa.size(), 0);
	for (int i = 2; i < oa.size(); i++)  {
		decompressArray(streams[i], in, oa[i]);
		fields[i] = oa[i].data();
	}

	records.resize(0);

	size_t stitchIdx = 0;
	uint32_t lastLoc = 0;
	for (size_t i = 0; i < sz; i++) {
		if (locations.data()[i] == 255)
			lastLoc = ((uint32_t*)stitches.data())[stitchIdx++];
		else
			lastLoc += locations.data()[i];
		records.add(getEditOperation(lastLoc, nucleotides, fields));
	}

	ZAMAN_END(EditOperation);

	recordCount = 0;
}

EditOperation EditOperationDecompressor::getEditOperation (size_t loc, ACTGStream &nucleotides, vector<uint8_t*> &fields) 
{
	ZAMAN_START(GetEO);

	EditOperation eo;
	eo.start = eo.end = loc;
	
	static Array<char> opChr(100, 100);
	static Array<int>  opLen(100, 100);

	opChr.resize(0);
	opLen.resize(0);

	size_t prevLoc = 0, endPos = 0; 
	int count = getEncoded(fields[EditOperationCompressor::Fields::OPLEN]) - 1;
	for (int i = 0; i < count; i++) {
		char c = *fields[EditOperationCompressor::Fields::OPCODES]++;
		assert(c);
		int l = 0;
		if (c == '*') { // Unmapped case. Just add * and exit
			assert(count == 1);
			endPos = getEncoded(fields[EditOperationCompressor::Fields::SEQEND]) - 1;
			opChr.add('*'), opLen.add(endPos);
			goto end;
		} 
		
		endPos += getEncoded(fields[EditOperationCompressor::Fields::SEQPOS]) - 1;
		if (c == '0') {
			c = *fields[EditOperationCompressor::Fields::OPCODES]++, l = 0; i++;
		} else if (c == 'X') {
			l = getEncoded(fields[EditOperationCompressor::Fields::XLEN]);
		} else if (c == 'H' || c == 'S') {
			l = getEncoded(fields[EditOperationCompressor::Fields::HSLEN]);
		} else {
			l = getEncoded(fields[EditOperationCompressor::Fields::LEN]);
		}

		// Do  we have trailing = ?
		if ((c == 'N' || c == 'D' || c == 'H' || c == 'P') && endPos > prevLoc) 
			opChr.add('='), opLen.add(endPos - prevLoc);
		else if (c != 'N' && c != 'D' && c != 'H' && c != 'P' && endPos - l > prevLoc) 
			opChr.add('='), opLen.add(endPos - l - prevLoc);
		prevLoc = endPos;
		opChr.add(c), opLen.add(l);
	}
	// End case. Check is prevLoc at end. If not, add = and exit
	endPos = getEncoded(fields[EditOperationCompressor::Fields::SEQEND]) - 1;
	if (!endPos)
		eo.seq = "*";
	else if (endPos > prevLoc)  
		opChr.add('='), opLen.add(endPos - prevLoc);

end:
	//for (int i = 0; i < opLen.size(); i++)
	//	LOG("%c %d", opChr[i], opLen[i]);

	size_t genPos = loc;
	char lastOP = 0;
	int  lastOPSize = 0;
	int mdOperLen = 0;
	eo.NM = 0;
	for (int i = 0; i < opChr.size(); i++) {
		// restore original part
		switch (opChr[i]) {
			case '=':
				if (lastOP == 'M')
					lastOPSize += opLen[i];
				else 
					lastOP = 'M', lastOPSize = opLen[i];
				
				for (int j = 0; j < opLen[i]; j++) {
					eo.seq += sequence[genPos + j];
					if (sequence[genPos + j] != sequence.getReference()[genPos + j]) {
						eo.MD += inttostr(mdOperLen), mdOperLen = 0;
						eo.MD += sequence.getReference()[genPos + j];
						eo.NM++;
					} else {
						mdOperLen++;
					}
				}
				genPos += opLen[i];
				break;

			case 'X':
				if (lastOP == 'M')
					lastOPSize += opLen[i];
				else 
					lastOP = 'M', lastOPSize = opLen[i];
				nucleotides.get(eo.seq, opLen[i]);

				for (int j = 0; j < opLen[i]; j++) {
					if (sequence.getReference()[genPos + j] != eo.seq[eo.seq.size() - opLen[i] + j]) {
						eo.MD += inttostr(mdOperLen), mdOperLen = 0;
						eo.MD += sequence.getReference()[genPos + j];
						eo.NM++;
					} else {
						mdOperLen++;
					}
				}

				genPos += opLen[i];
				break;

			case 'I':
				eo.NM += opLen[i];
			case '*':
			case 'S':
				nucleotides.get(eo.seq, opLen[i]);
			case 'H':
			case 'P':
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				if (opChr[i] == '*') {
					eo.op = "*";
				} else {
					eo.op += inttostr(opLen[i]) + char(opChr[i]);
				}
				break;

			case 'D':
				eo.MD += inttostr(mdOperLen), mdOperLen = 0;
				eo.MD += "^";
				for (int j = 0; j < opLen[i]; j++)
					eo.MD += sequence.getReference()[genPos + j]; 
				eo.NM += opLen[i];
			case 'N': 
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				eo.op += inttostr(opLen[i]) + char(opChr[i]);
				eo.end += opLen[i];
				genPos += opLen[i];
				break;
		}
	}
	if (lastOP != 0) {
		eo.op += inttostr(lastOPSize) + lastOP;
		if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
			eo.end += lastOPSize;
	}
	if (mdOperLen || !isdigit(eo.MD.back())) eo.MD += inttostr(mdOperLen);
	if (!isdigit(eo.MD[0])) eo.MD = "0" + eo.MD;

	if (eo.seq == "" || eo.seq[0] == '*')
		eo.seq = "*";

	ZAMAN_END(GetEO);
	return eo;
}

void EditOperationDecompressor::setIndexData (uint8_t *in, size_t in_size) 
{
	streams[EditOperationCompressor::Fields::LOCATION]->setCurrentState(in, in_size);
}
