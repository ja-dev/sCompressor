# **sCompressor 0.1**

Simple dictionary based instruction-compressor. This program takes in 32 bit binary instructions, builds a dictionary based on pre-set configurations, and chooses the best from 8 different compression techniques for each line of instruction to be compressed. 

Description taken from project assignment description:
Assume that the dictionary can have sixteen entries (index 4 bits) and the sixteen entries are
selected based on frequency (the most frequent instruction should have index 0000). If two
entries have the same frequency, priority is given to the one that appears first in the original
program order. The original code consists of 32-bit binaries. You are allowed to use only eight
possible formats for compression (as outlined below). Note that if one entry (32-bit binary) can
be compressed in more than one way, choose the most beneficial one i.e., the one that provides
the shortest compressed pattern. If two formats produce exactly the same compression, choose
the one that appears earlier in the following listing (e.g., run-length encoding appears earlier than
direct matching). If a 32-bit binary can be compressed using multiple dictionary entries by any
specific compression format (e.g., bitmask-based compression), please use the dictionary entry
with the smallest index value. Please count the starting location of a mismatch from the leftmost
(MSB) bit of the pattern – the position of the leftmost bit is 00000.

Each instruction is chosen from the best of the following:
![alt tag](https://github.com/ja-dev/sCompressor/blob/master/03_Screenshots/02_Compression%20Techniques.jpg)

Compression/decompression:
![alt tag](https://github.com/ja-dev/sCompressor/blob/master/03_Screenshots/01_Compression.jpg)
