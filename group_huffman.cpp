#include "group_huffman.h"
namespace THitNumCompressor {
#include "hitnum.inc"
};
namespace THitsCompressor {
#include "hits.inc"
};
namespace TDocIdCompressor {
#include "docid.inc"
};

THuffGroupCompressor hitNumCompressor(
                                      THitNumCompressor::hff, 
                                      ARRAY_SIZE(THitNumCompressor::hff), 
                                      THitNumCompressor::lev, 
                                      ARRAY_SIZE(THitNumCompressor::lev),
                                      THitNumCompressor::inv,
                                      THitNumCompressor::dir);

THuffGroupCompressor hitsCompressor(
                                      THitsCompressor::hff, 
                                      ARRAY_SIZE(THitsCompressor::hff), 
                                      THitsCompressor::lev, 
                                      ARRAY_SIZE(THitsCompressor::lev),
                                      THitsCompressor::inv,
                                      THitsCompressor::dir);

THuffGroupCompressor docidCompressor(
                                      TDocIdCompressor::hff, 
                                      ARRAY_SIZE(TDocIdCompressor::hff), 
                                      TDocIdCompressor::lev, 
                                      ARRAY_SIZE(TDocIdCompressor::lev),
                                      TDocIdCompressor::inv,
                                      TDocIdCompressor::dir);

THuffGroupDecompressor hitNumDecompressor(
                                      THitNumCompressor::hff, 
                                      ARRAY_SIZE(THitNumCompressor::hff), 
                                      THitNumCompressor::lev, 
                                      ARRAY_SIZE(THitNumCompressor::lev),
                                      THitNumCompressor::inv,
                                      THitNumCompressor::dir);

THuffGroupDecompressor hitsDecompressor(
                                      THitsCompressor::hff, 
                                      ARRAY_SIZE(THitsCompressor::hff), 
                                      THitsCompressor::lev, 
                                      ARRAY_SIZE(THitsCompressor::lev),
                                      THitsCompressor::inv,
                                      THitsCompressor::dir);

THuffGroupDecompressor docidDecompressor(
                                      TDocIdCompressor::hff, 
                                      ARRAY_SIZE(TDocIdCompressor::hff), 
                                      TDocIdCompressor::lev, 
                                      ARRAY_SIZE(TDocIdCompressor::lev),
                                      TDocIdCompressor::inv,
                                      TDocIdCompressor::dir);