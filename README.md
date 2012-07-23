Index compression codecs playground
===================================

Test implementations of various full text index compression techniques.

What's this? What's this? There's color everywhere!
---------------------------------------------------

Basically, (nice and clean) compression code.
And testing data that accompanies it.

### The Problem

Full text indexes come with a special structure and requirements that require
special compression schemes. Decompression speed and (then) index size are vital.
This project should be an easy playground to work on those codecs.

### The Code

Currently, a baseline Varint codec, and a fancy Huffman based codec.
We plan to add more, but feel free to fork and preempt us.

### The Data

Actual postings data, collected by indexing 150,000 Wikipedia documents.
Note that we indexed the source text with Wiki markup, **not** the HTML versions.

Test postings data
------------------

http://narod.ru/disk/57326648001.2dd50c0c6b695bfa14cbeb0be8fdb9fb/postings-wiki.7z.html

docids.bin, 600000 bytes, MD5 fc11c8c9a9c751d7582240d87eaeb989

postings.bin, 2165395590 bytes, MD5 f0ce38606483e111963dda60ec1c5851
