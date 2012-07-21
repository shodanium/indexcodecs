#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <time.h>

typedef unsigned char ui8;
typedef unsigned short ui16;
typedef unsigned int ui32;
typedef unsigned long long ui64;

struct THuffEntry {
	ui32 CodeBase;
	ui16 Prefix;
	ui8 PrefLength;
	ui8 CodeLength;
	bool inline Fit(ui32 code) const {
		return code >= CodeBase && code < CodeBase + (1ULL << CodeLength);
	}
};

template<size_t Size>
struct THuffDecompressor {
	THuffDecompressor(const THuffEntry *entries, size_t entryNum) {
		for (size_t i = 0; i < Size; ++i) {
			bool init = false;
			for (size_t j = 0; j < entryNum; ++j) {
				const THuffEntry &entry = entries[j];
				ui16 prefix = i & ((1 << entry.PrefLength) - 1);
				if (entry.Prefix == prefix) {
					init = true;
					Mask[i] = (ui32)((1ULL << entry.CodeLength) - 1ULL);
					Add[i] = entry.PrefLength + entry.CodeLength; 
					PrefLength[i] = entry.PrefLength;
					CodeBase[i] = entry.CodeBase;
					break;
				}
			}
			assert(init);
		}
	}
	ui32 Mask[Size];
	ui32 Add[Size];
	ui32 PrefLength[Size];
	ui32 CodeBase[Size];
	ui32 Decompress(const ui8 *codes, ui64 &offset) {
		ui64 code = (*((const ui64 *)(codes + (offset >> 3)))) >> (offset & 7);
		ui64 index = code % Size;
		offset += Add[index];
		return ui32(((code >> PrefLength[index]) & Mask[index])) + CodeBase[index];
	}
};

template<size_t Size>
struct THuffCompressor {

	const THuffEntry *Entries;
	size_t EntryNum;
	ui64 Count;

	struct THashEntry {
		ui32 Value;
		ui32 CodeSize;
		ui64 Code;
	};

	THashEntry Hashes[Size];

	size_t Hash(ui32 value) {
		return value % Size;
	}

	THuffCompressor(const THuffEntry *entries, size_t entryNum) 
		: Entries(entries)
		, EntryNum(entryNum)
		, Count(0)
	{
		for (size_t i = 0; i < Size; ++i) {
			ui32 j = 0;
			for (; Hash(j) == i; ++j){}
			Hashes[i].Value = j;
		}
	}

	const THashEntry &GetCode(ui32 value) {
		THashEntry &code = Hashes[Hash(value)];
		if (code.Value == value)
			return code;

		code.Value = value;
		for (size_t i = 0; i < EntryNum; ++i) {
			const THuffEntry &entry = Entries[i];
			if (Entries[i].Fit(value)) {
				code.CodeSize = entry.CodeLength + entry.PrefLength;
				code.Code = ((ui64(value) - entry.CodeBase) << entry.PrefLength) + entry.Prefix;
				return code;
			}
		}
		code.Code = 0;
		code.CodeSize = 0;
		assert(0);
		return code;
	}

	void Code(std::vector<ui8> &data, ui32 value, ui64 &position) {
		++Count;
		const THashEntry &entry = GetCode(value);
		size_t index = (size_t)(position >> 3);
		if (data.size() < index + 8) {
			data.resize((data.size() + 8) * 2, 0);
		}
		ui32 size = entry.CodeSize;
		ui64 code = entry.Code;
		ui8 *dst = &data[index];
		ui64 &outCode = *(ui64 *)dst;
		ui8 shift = position & 7;
		ui64 mask = ((1ULL << size) - 1ULL) << shift;
		outCode = ((code << shift) & mask) | (outCode & ~mask);
		position += size;
	}
};


const THuffEntry docEntries[] = {
{0x00000001, 0x00, 0x02, 0x00},
{0x00000002, 0x0f, 0x04, 0x00},
{0x00000003, 0x05, 0x04, 0x00},
{0x00000004, 0x13, 0x05, 0x00},
{0x00000005, 0x1a, 0x05, 0x00},
{0x00000006, 0x12, 0x05, 0x00},
{0x00000000, 0x01, 0x03, 0x04},
{0x00000010, 0x0d, 0x04, 0x04},
{0x00000020, 0x03, 0x05, 0x04},
{0x00000030, 0x0a, 0x05, 0x04},
{0x00000000, 0x06, 0x03, 0x08},
{0x00000100, 0x07, 0x05, 0x08},
{0x00000000, 0x0b, 0x04, 0x0c},
{0x00000000, 0x17, 0x05, 0x10},
{0x00000000, 0x02, 0x05, 0x14}
};

const THuffEntry cntEntries[] = {
{0x00000001, 0x01, 0x01, 0x00},
{0x00000002, 0x00, 0x02, 0x00},
{0x00000003, 0x0e, 0x04, 0x00},
{0x00000004, 0x0a, 0x04, 0x00},
{0x00000005, 0x12, 0x05, 0x00},
{0x00000000, 0x06, 0x04, 0x04},
{0x00000000, 0x02, 0x05, 0x19}
};

const THuffEntry posEntries[] = {
{0x00000000, 0x07, 0x03, 0x04},
{0x00000010, 0x04, 0x03, 0x04},
{0x01000000, 0x06, 0x04, 0x04},
{0x00000020, 0x02, 0x04, 0x04},
{0x01000010, 0x08, 0x04, 0x04},
{0x01000000, 0x01, 0x02, 0x08},
{0x00000000, 0x03, 0x03, 0x08},
{0x01000100, 0x0e, 0x04, 0x08},
{0x01000000, 0x0a, 0x04, 0x0c},
{0x00000000, 0x10, 0x05, 0x18},
{0x00000000, 0x00, 0x05, 0x19},
};

struct IndexCompressor {
	ui8 Name[256];
	ui8 Length;
	ui32 Doc;
	ui32 Pos;
	std::vector<ui8> Code;
	ui64 Position;
	struct TDoc {
		ui32 Id;
		std::vector<ui32> PostingList;
	};
	ui8 Count;
	std::vector<TDoc> Docs;

	THuffCompressor<1024> DocCompressor;
	THuffCompressor<128> CntCompressor;
	THuffCompressor<4096> PosCompressor;
	THuffCompressor<128> WrdCompressor;

	THuffDecompressor<32> DocDecompressor;
	THuffDecompressor<32> CntDecompressor;
	THuffDecompressor<32> PosDecompressor;
	THuffDecompressor<32> WrdDecompressor;

	
	IndexCompressor() 
		: Position(0)
		, DocCompressor(docEntries, sizeof(docEntries) / sizeof(docEntries[0]))
		, CntCompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
		, PosCompressor(posEntries, sizeof(posEntries) / sizeof(posEntries[0]))
		, WrdCompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
		, DocDecompressor(docEntries, sizeof(docEntries) / sizeof(docEntries[0]))
		, CntDecompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
		, PosDecompressor(posEntries, sizeof(posEntries) / sizeof(posEntries[0]))
		, WrdDecompressor(cntEntries, sizeof(cntEntries) / sizeof(cntEntries[0]))
	{
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


	void Flush() {
		if (Docs.empty())
			return;
		if (++Count == 0) {
			fprintf(stdout, ".");
			fflush(stdout);
		}

		WrdCompressor.Code(Code, Docs.size(), Position);
		ui32 oldDoc = ui32(-1);
		for (size_t i = 0; i < Docs.size(); ++i) {
			const TDoc &doc = Docs[i];
			DocCompressor.Code(Code, doc.Id - oldDoc, Position);
			oldDoc = doc.Id;
			CntCompressor.Code(Code, doc.PostingList.size(), Position);
			ui32 oldPosting = ui32(-1);
			for (size_t j = 0; j < doc.PostingList.size(); ++j) {
				PosCompressor.Code(Code, doc.PostingList[j] - oldPosting, Position);
				oldPosting = doc.PostingList[j];
			}
		}
		Docs.clear();
	}

	bool Read(FILE *fp) {
		static int a = 0;
		if (!feof(fp)) {
			ui8 type;
			fread(&type, 1, 1, fp);
			if (type) {
				Flush();
				fread(&Length, 1, 1, fp);
				fread(Name, 1, Length, fp);
				Name[(size_t)Length + 1] = 0;
			}
			fread(&Doc, 4, 1, fp);
			fread(&Pos, 4, 1, fp);
			return !feof(fp);
		}
		return false;
	}

	void Compress(FILE *fp) {
		while (Read(fp)) {
			if (Docs.empty() || Docs.back().Id != Doc) {
				Docs.push_back(TDoc());
				Docs.back().Id = Doc;
			}
			Docs.back().PostingList.push_back(Pos);
		}
		Flush();
		printf("\n");
		printf("Bytes %lld\n", (long long)Position / 8);
		printf("Words %lld\n", (long long)WrdCompressor.Count);
		printf("Docs %lld\n", (long long)DocCompressor.Count);
		printf("Postings %lld\n", (long long)PosCompressor.Count);
	};
};

int main(int argc, const char *argv[]) {
	if (argc < 2)
		return 1;
	FILE *fp = fopen(argv[1], "rb");
	if (!fp) {
		return 1;
	}

	IndexCompressor comp;
	comp.Compress(fp);
	float cl1 = clock();
	comp.Decompress();
	float cl2 = clock();
	printf("decompress time %f seconds\n", (cl2 - cl1) / (float)CLOCKS_PER_SEC);
}