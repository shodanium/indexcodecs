#pragma once
#include "common.h"

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
		ui32 index = code % Size;
		offset += Add[index];
		return (ui32(code >> PrefLength[index]) & Mask[index]) + CodeBase[index];
	}
};

struct THashEntry {
    ui64 Code;
    ui32 Value;
    ui16 CodeSize;
    ui16 PrefNum;
};

template<size_t Size>
struct THuffCompressor {
	const THuffEntry *Entries;
	size_t EntryNum;
	ui64 Count;

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
				code.PrefNum = (ui16)i;
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

    void CodeNoCheck(std::vector<ui8> &data, ui32 size, ui64 code, ui64 &position) {
		size_t index = (size_t)(position >> 3);
        ui8 *dst = &data[index];
		ui64 &outCode = *(ui64 *)dst;
		ui8 shift = position & 7;
		ui64 mask = ((1ULL << size) - 1ULL) << shift;
		outCode = ((code << shift) & mask) | (outCode & ~mask);
		position += size;
    }

	void Code(std::vector<ui8> &data, ui32 value, ui64 &position) {
		++Count;
		const THashEntry &entry = GetCode(value);
		size_t index = (size_t)(position >> 3);
		if (data.size() < index + 8) {
			data.resize((data.size() * 3 / 2) + 8, 0);
		}
        CodeNoCheck(data, entry.CodeSize, entry.Code, position);
	}
};

struct THuffGroupDecompressor {
    struct TUPackInfo {     
        ui32 CodeBase[4];
        ui32 Offset[4];
    };

    TUPackInfo Infos[10000];
    THuffDecompressor<64> IndexDecompressor;
    THuffDecompressor<64> ValueDecompressor;
    THuffGroupDecompressor(
        const THuffEntry *entries, size_t entryNum, 
        const THuffEntry *levels, size_t levelNum, const int *inv, const int *dir) 
            : IndexDecompressor(entries, entryNum)
            , ValueDecompressor(levels, levelNum) {
        for (size_t i = 0; i < 10000; ++i) {
            ui32 val = dir[i];
            for (size_t j = 0; j < 4; ++j) {
                ui32 v = (val % 10);
                val /= 10;
                Infos[i].CodeBase[j] = levels[v].CodeBase;
                Infos[i].Offset[j] = levels[v].CodeLength;
            }
        }
    }

    ui32 Fetch(const ui8 *codes, ui64 offset) {
        ui64 code = (*((const ui64 *)(codes + (offset >> 3)))) >> (offset & 7);
		return (ui32)code;
    }

    void Decompress(const ui8 *codes, ui64 &offset, volatile ui32 *result) {
        ui32 tableIndex = IndexDecompressor.Decompress(codes, offset);
        const TUPackInfo &info = Infos[tableIndex];
        
        ui32 off0 = info.Offset[0];
        ui32 off1 = info.Offset[1];
        ui32 off2 = info.Offset[2];
        ui32 off3 = info.Offset[3];

        ui32 m0 = (1ULL << off0) - 1UL;
        ui32 m1 = (1ULL << off1) - 1UL;
        ui32 m2 = (1ULL << off2) - 1UL;
        ui32 m3 = (1ULL << off3) - 1UL;

        result[0] = (Fetch(codes, offset) & m0) + info.CodeBase[0];
        offset += off0;
        result[1] = (Fetch(codes, offset) & m1) + info.CodeBase[1];
        offset += off1;
        result[2] = (Fetch(codes, offset) & m2) + info.CodeBase[2];
        offset += off2;
        result[3] = (Fetch(codes, offset) & m3) + info.CodeBase[3];
        offset += off3;
	}
};

extern THuffGroupDecompressor docidDecompressor;
extern THuffGroupDecompressor hitsDecompressor;
extern THuffGroupDecompressor hitNumDecompressor;


struct THuffGroupCompressor {
    THuffCompressor<4096> IndexCompressor;
    THuffCompressor<4096> ValueCompressor;
    const int *Inv;
    const int *Dir;
    const THuffEntry *Levels;
    THuffGroupCompressor(
        const THuffEntry *entries, size_t entryNum, 
        const THuffEntry *levels, size_t levelNum, const int *inv, const int *dir) 
        : IndexCompressor(entries, entryNum)
        , ValueCompressor(levels, levelNum)
        , Inv(inv)
        , Dir(dir)
        , Ptr(0)
    {
    }
    ui32 Buff[4];
    size_t Ptr;
    void Flush(std::vector<ui8> &data, ui64 &position) {
        if (Ptr != 0)
            Code4(data, Buff[3], Buff[2], Buff[1], Buff[0], position);
        Ptr = 0;
    }
    void Add(std::vector<ui8> &data, ui32 code, ui64 &position) {
        Buff[Ptr] = code;
        ++Ptr;
        if (Ptr == 4)
            Flush(data, position);
    }
    void Code4(std::vector<ui8> &data, ui32 v0, ui32 v1, ui32 v2, ui32 v3, ui64 &position) {
        THashEntry h0 = ValueCompressor.GetCode(v0);
        THashEntry h1 = ValueCompressor.GetCode(v1);
        THashEntry h2 = ValueCompressor.GetCode(v2);
        THashEntry h3 = ValueCompressor.GetCode(v3);
        ui32 key = h0.PrefNum + h1.PrefNum * 10 + h2.PrefNum * 100 + h3.PrefNum * 1000;
        ui32 recodedKey = Inv[key];
        THashEntry hv = IndexCompressor.GetCode(recodedKey);
        size_t index = (size_t)(position >> 3);
        if (data.size() < index + 128) {
			data.resize((data.size() * 3 / 2) + 128, 0);
		}
        IndexCompressor.CodeNoCheck(data, hv.CodeSize, hv.Code, position);
        ValueCompressor.CodeNoCheck(data, h0.CodeSize, h0.Code, position);
        ValueCompressor.CodeNoCheck(data, h1.CodeSize, h1.Code, position);
        ValueCompressor.CodeNoCheck(data, h2.CodeSize, h2.Code, position);
        ValueCompressor.CodeNoCheck(data, h3.CodeSize, h3.Code, position);
    }
};

extern THuffGroupCompressor docidCompressor;
extern THuffGroupCompressor hitsCompressor;
extern THuffGroupCompressor hitNumCompressor;
