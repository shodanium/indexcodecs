
// cl /O2 /EHsc test.cpp
// test postings.bin
//////////////////////////////////////////////////////////////////////////
#include "common.h"
#include "group_huffman.h"

struct Codec
{
public:
	/// Read() fills these
	ui8 Name[256];	///< current keyword
	ui8 Length;		///< current keyword length
	ui32 Doc;		///< current docid
	ui32 Pos;		///< current position

	/// Compress() fills this
	struct TDoc {
		ui32 Id;
		std::vector<ui32> PostingList;
	};
	std::vector<TDoc> Docs;		///< current docid, and a positions list
    int Progress;
	vector<ui32> Docids;

protected:
	ui8 Buf[262144];
	ui8 * BufMax;
	ui8 * BufCur;

public:
	Codec()
		: Progress(0)
		, BufMax(Buf + sizeof(Buf))
		, BufCur(Buf + sizeof(Buf))
	{
		FILE * fp = fopen("docids.bin", "rb+");
		if (!fp)
			die("failed to read docids.bin");
		fseek(fp, 0, SEEK_END);
		Docids.resize(ftell(fp) / 4);
		fseek(fp, 0, SEEK_SET);
		fread(&Docids[0], 4, Docids.size(), fp);
		fclose(fp);
		sort(Docids.begin(), Docids.end());
	}

	inline ui32 RemapId(ui32 Id) const
	{
		const ui32 * pId = lower_bound(&Docids[0], &Docids[0]+Docids.size(), Id);
#if 0
		if (!pId || *pId!=Id)
			die("oops, docid not found");
#endif
		return pId - &Docids[0] + 1;
	}

	virtual void FlushWord() = 0;
	virtual void PostCompress() = 0;
	virtual void Decompress() = 0;

public:
	void FlushWordInt() {
		if (Docs.empty())
			return;
		if (!(++Progress%1000))
		{
			printf("%d\r", Progress);
			fflush(stdout);
		}
		FlushWord();
		Docs.resize(0);
    }

	bool Read(FILE *fp)
	{
		static int a = 0;

		// are we totally out of data? read in some more
		if (BufCur>=BufMax)
		{
			int Got = fread(Buf, 1, sizeof(Buf), fp);
			if (!Got)
				return false;
			BufCur = Buf;
			BufMax = Buf+Got;
		}

		// is the current entry complete?
		ui8 type = *BufCur++;
		if ((type==0 && BufCur+8>BufMax)
			|| (type==1 && BufCur+9+BufCur[0]>BufMax))
		{
			int Left = BufMax-BufCur;
			memmove(Buf, BufCur, Left);
			int Got = fread(Buf+Left, 1, sizeof(Buf)-Left, fp);
			if (!Got)
				return false;
			BufCur = Buf;
			BufMax = Buf+Left+Got;
		}

		// safe to keep reading now
		if (type)
		{
			FlushWordInt();
			Length = *BufCur++;
			memcpy(Name, BufCur, Length);
			Name[(size_t)Length + 1] = 0;
			BufCur += Length;
		}

		Doc = *(ui32*)BufCur;
		Pos = *(ui32*)(BufCur+4);
		BufCur += 8;
		return true;
	}

	void Compress(FILE *fp) {
		while (Read(fp)) {
			if (Docs.empty() || Docs.back().Id != Doc) {
				Docs.push_back(TDoc());
				Docs.back().Id = Doc;
			}
			Docs.back().PostingList.push_back(Pos);
		}
		FlushWordInt();
		Docs.clear();
	}
};

//////////////////////////////////////////////////////////////////////////
const THuffEntry docEntries[] = {
{0x00000001, 0x00, 0x02, 0x00},
{0x00000002, 0x07, 0x04, 0x00},
{0x00000003, 0x01, 0x04, 0x00},
{0x00000004, 0x0b, 0x05, 0x00},
{0x00000005, 0x03, 0x05, 0x00},
{0x00000006, 0x16, 0x05, 0x00},
{0x00000000, 0x05, 0x03, 0x04},
{0x00000010, 0x0f, 0x04, 0x04},
{0x00000020, 0x1b, 0x05, 0x04},
{0x00000030, 0x09, 0x05, 0x04},
{0x00000000, 0x02, 0x03, 0x08},
{0x00000100, 0x19, 0x05, 0x08},
{0x00000000, 0x0e, 0x04, 0x0c},
{0x00000000, 0x13, 0x05, 0x10},
{0x00000000, 0x06, 0x05, 0x12},
};

const THuffEntry cntEntries[] = {
{0x00000001, 0x01, 0x01, 0x00},
{0x00000002, 0x00, 0x02, 0x00},
{0x00000003, 0x06, 0x04, 0x00},
{0x00000004, 0x1a, 0x05, 0x00},
{0x00000005, 0x0a, 0x05, 0x00},
{0x00000000, 0x0e, 0x04, 0x04},
{0x00000010, 0x12, 0x05, 0x04},
{0x00000000, 0x02, 0x05, 0x12},
};

const THuffEntry posEntries[] = {
{0x00000003, 0x11, 0x05, 0x00},
{0x00000004, 0x06, 0x05, 0x00},
{0x00000005, 0x08, 0x05, 0x00},
{0x00000000, 0x07, 0x03, 0x04},
{0x00000010, 0x05, 0x03, 0x04},
{0x00000020, 0x03, 0x04, 0x04},
{0x00000030, 0x00, 0x04, 0x04},
{0x00000040, 0x16, 0x05, 0x04},
{0x00000000, 0x02, 0x03, 0x08},
{0x01000000, 0x09, 0x04, 0x08},
{0x01000100, 0x1b, 0x05, 0x08},
{0x00000100, 0x0b, 0x05, 0x08},
{0x01000200, 0x18, 0x05, 0x08},
{0x01000000, 0x04, 0x03, 0x0c},
{0x00000000, 0x0e, 0x04, 0x0c},
{0x00000000, 0x01, 0x05, 0x19},
};


struct HuffCodec : public Codec
{
	std::vector<ui8> Code;
	ui64 Position;
	ui8 Count;

	THuffCompressor<1024> DocCompressor;
	THuffCompressor<128> CntCompressor;
	THuffCompressor<4096> PosCompressor;
	THuffCompressor<128> WrdCompressor;

	THuffDecompressor<32> DocDecompressor;
	THuffDecompressor<32> CntDecompressor;
	THuffDecompressor<32> PosDecompressor;
	THuffDecompressor<32> WrdDecompressor;

	HuffCodec()
		: Position(0)
		, DocCompressor(docEntries, sizeof(docEntries) / sizeof(docEntries[0]))
		, CntCompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
		, PosCompressor(posEntries, sizeof(posEntries) / sizeof(posEntries[0]))
		, WrdCompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
		, DocDecompressor(docEntries, sizeof(docEntries) / sizeof(docEntries[0]))
		, CntDecompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
		, PosDecompressor(posEntries, sizeof(posEntries) / sizeof(posEntries[0]))
		, WrdDecompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
	{}

	void FlushWord() {
		WrdCompressor.Code(Code, Docs.size(), Position);
		ui32 oldDoc = ui32(-1);
		for (size_t i = 0; i < Docs.size(); ++i) {
			const TDoc &doc = Docs[i];
            ui32 uId = RemapId(doc.Id);
			DocCompressor.Code(Code, uId - oldDoc, Position);
			oldDoc = uId;
			CntCompressor.Code(Code, doc.PostingList.size(), Position);
			ui32 oldPosting = ui32(-1);
			for (size_t j = 0; j < doc.PostingList.size(); ++j) {
				PosCompressor.Code(Code, doc.PostingList[j] - oldPosting, Position);
				oldPosting = doc.PostingList[j];
			}
		}
	}

	void PostCompress()
	{
		printf("\n");
		printf("Bytes %lld\n", (long long)Position / 8);
		printf("Words %lld\n", (long long)WrdCompressor.Count);
		printf("Docs %lld\n", (long long)DocCompressor.Count);
		printf("Postings %lld\n", (long long)PosCompressor.Count);
	}

	void Decompress() {
		if (Code.empty())
			return;
		ui64 position = 0;
		const ui8 *code = &Code[0];
		long long words = 0;
		long long docs = 0;
		long long pos = 0;

		while (position < Position) {
			ui32 count = WrdDecompressor.Decompress(code, position);
			ui32 doc = -1;
			++words;
			for (ui32 i = 0; i < count; ++i) {
				doc += DocDecompressor.Decompress(code, position);
				ui32 postings = CntDecompressor.Decompress(code, position);
				ui32 posting = -1;
				++docs;
				for (ui32 j = 0; j < postings; ++j) {
					++pos;
					posting += PosDecompressor.Decompress(code, position);
				}
			}
		}

		printf("%lld %lld %lld\n", words, docs, pos);
	}
};

//////////////////////////////////////////////////////////////////////////

void VarintCode(vector<ui8> & buf, ui32 value)
{
	if (value >= (1UL<<7))
	{
		if (value >= (1UL<<14))
		{
			if (value >= (1UL<<28))
			{
				buf.push_back(0x80 | (value>>28));
				buf.push_back(0x80 | (value>>21));
			} else if (value >= (1UL<<21))
			{
				buf.push_back(0x80 | (value>>21));
			}
			buf.push_back(0x80 | (value>>14));
		}
		buf.push_back(0x80 | (value>>7));
	}
	buf.push_back(0x7f & value);
	return;
}


ui32 VarintDecode(const ui8 ** pp)
{
	const ui8 * p = *pp;
	ui32 res = 0;
	while ( *p & 0x80 )
		res = (res<<7) + ((*p++) & 0x7f);
	res = (res<<7) + (*p++);
	*pp = p;
	return res;
}


struct VarintCodec : public Codec
{
	vector<ui8> Dict;
	vector<ui8> Data;
	int PackedWords;
	int PackedDocs;
	int PackedHits;

	VarintCodec()
		: PackedWords(0)
		, PackedDocs(0)
		, PackedHits(0)
	{}

	virtual void FlushWord()
	{
		int p = Dict.size();
		int t = Data.size();

		// dict entry
		PackedWords++;
		Dict.resize(p + Length + 5);
		Dict[p] = Length; // keyword
		memcpy(&Dict[p+1], Name, Length);
		memcpy(&Dict[p+1+Length], &t, 4); // offset into data

		// documents and hits
		PackedDocs += Docs.size();
		ui32 uLastId = 0;
		for (int i=0; i<Docs.size(); i++)
		{
			ui32 uId = RemapId(Docs[i].Id);
			VarintCode(Data, uId - uLastId);
			uLastId = uId;

			const vector<ui32> & hits = Docs[i].PostingList;
			PackedHits += hits.size();
			VarintCode(Data, hits.size());

			ui32 uLastHit = 0;
			for (int j=0; j<hits.size(); j++)
			{
				VarintCode(Data, hits[j] - uLastHit);
				uLastHit = hits[j];
			}
		}
		VarintCode(Data, 0);
	}

	virtual void PostCompress()
	{
		printf("total %lu bytes (%lu dict, %lu data), %d words\n",
			Dict.size() + Data.size(), Dict.size(), Data.size(), Progress);
		printf("packed %d words, %d docs, %d hits\n",
			PackedWords, PackedDocs, PackedHits);
	}

	virtual void Decompress()
	{
		int words = 0, docs = 0, hits = 0;
		const ui8 * w = &Dict[0];
		const ui8 * wmax = w + Dict.size();
		while (w<wmax)
		{
			words++;
			w += 1+w[0]; // skip keyword

			ui32 dataOffset = *(ui32*)w;
			w += 4;

			const ui8 * p = &Data[dataOffset];
			ui32 uLastId = 0;
			for (;;)
			{
				ui32 uDelta = VarintDecode(&p);
				if (!uDelta)
					break;

				uLastId += uDelta;
				docs++;

				ui32 uDocHits = VarintDecode(&p);
				hits += uDocHits;

				ui32 uHit = 0;
				for (int j=0; j<uDocHits; j++)
					uHit += VarintDecode(&p);
			}
		}
		if ( w!=wmax )
			printf("oops, decompress screwed up\n");
		printf("unpacked %d words, %d docs, %d hits\n", words, docs, hits);
	}

	void Save()
	{
		FILE * fp = fopen("postings.varint", "wb+");
		if (!fp)
			die("failed to write postings.varint");
		int iLen = Dict.size();
		fwrite(&iLen, 1, 4, fp);
		fwrite(&Dict[0], 1, iLen, fp);
		iLen = Data.size();
		fwrite(&iLen, 1, 4, fp);
		fwrite(&Data[0], 1, iLen, fp);
		fclose(fp);
	}

	void Load()
	{
		FILE * fp = fopen("postings.varint", "rb");
		if (!fp)
			die("failed to read postings.varint");
		int iLen;
		fread(&iLen, 1, 4, fp);
		Dict.resize(iLen);
		if (fread(&Dict[0], 1, iLen, fp)!=iLen)
			die("dict read failed");
		fread(&iLen, 1, 4, fp);
		Data.resize(iLen);
		if (fread(&Data[0], 1, iLen, fp)!=iLen)
			die("data read failed");
		fclose(fp);
	}
};

//////////////////////////////////////////////////////////////////////////
struct GroupHuffCodec : public Codec
{
    std::vector<ui8> DocData;
    ui64 DocPosition;
    std::vector<ui8> HitData;
    ui64 HitPosition;
    ui64 DocNum;
    ui64 HitNum;
    

    GroupHuffCodec()
        : DocPosition(0)
        , HitPosition(0)
        , DocNum(0)
        , HitNum(0)
    {
    }


    virtual void FlushWord() {
        ui32 uLastId = -1;
        std::vector<ui8> data;
        ui64 position = 0;
        for (int i=0; i<Docs.size(); i++)
        {
            ui32 uId = RemapId(Docs[i].Id);
            docidCompressor.Add(DocData, uId - uLastId, DocPosition);
            uLastId = uId;
            ui32 uLastHit = 1 << 24;
            const vector<ui32> & hits = Docs[i].PostingList;
            ++DocNum;
            hitNumCompressor.Add(DocData, hits.size(), DocPosition);
            for (int j=0; j<hits.size(); j++)
            {
                ++HitNum;
                hitsCompressor.Add(HitData, hits[j] - uLastHit, HitPosition);
                uLastHit = hits[j];
            }
        }
    }
    virtual void PostCompress() {
        docidCompressor.Flush(DocData, DocPosition);
        hitNumCompressor.Flush(DocData, DocPosition);
        hitsCompressor.Flush(HitData, HitPosition);
        printf("docs %lld hits %lld total %lld\n", DocPosition / 8, HitPosition / 8, DocPosition / 8 + HitPosition / 8);
        printf("packed docs %lld packed hits %lld\n", DocNum, HitNum);
    }
    virtual void Load() {}
    virtual void Save() {}
	virtual void Decompress() {
        ui64 docPosition = 0;
        const ui8 *docs = &DocData[0];
        ui64 docNum = 0;
        while (docPosition < DocPosition) {
            volatile ui32 docids[4];
            volatile ui32 hitnums[4];
            docNum += 4;
            docidDecompressor.Decompress(docs, docPosition, docids);
            hitNumDecompressor.Decompress(docs, docPosition, hitnums);
        }
        ui64 hitPosition = 0;
        const ui8 *hits = &HitData[0];
        ui64 hitNum = 0;
        while (hitPosition < HitPosition) {
            volatile ui32 hit[4];
            hitNum += 4;
            hitsDecompressor.Decompress(hits, hitPosition, hit);
        }
        printf("unpacked docs %lld hits %lld\n", docNum, hitNum); 
    }
};


//////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		printf("usage: test <postings.file>\n");
		return 1;
	}
	FILE *fp = fopen(argv[1], "rb");
	if (!fp)
	{
		printf("failed to to open %s\n", argv[1]);
		return 1;
	}

	try
	{

		GroupHuffCodec comp;
        float t;


		t = clock();
		comp.Compress(fp);
		printf("compress time %f seconds\n", (clock() - t) / (float)CLOCKS_PER_SEC);
		comp.PostCompress();
		comp.Save();

		comp.Load();
		t = clock();
		comp.Decompress();
		printf("decompress time %f seconds\n", (clock() - t) / (float)CLOCKS_PER_SEC);
	} catch (exception & e)
	{
		printf("exception: %s\n", e.what());
	}
}
