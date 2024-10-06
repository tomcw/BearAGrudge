# BearAGrudge

Compression and decompression is done with [Exomizer](https://bitbucket.org/magli143/exomizer/wiki/Home) v3.1.1.

As chunk size gets smaller, then compression gets slightly worse:

chunks | chunk size | total compressed size
-|-|-
1| 55,275 | 7,551
7| 8,184 x 6 + 6,171 | 8,342
8| 7,163 x 7 + 5,150(*) | 9,148

(*) total = 55,275 + 16: the extra 16 = 8x{FF,FF} end marker bytes.

Pre-pack the 11 bytes of AY regs into 8 bytes:

chunks | chunk size | total compressed size
-|-|-
1| 40,202 | 7,382
5| 8,186 x 4 + 7,466 | 7,933 (used this)
6| 7,170 x 5 + 4,362 | 8,480
6| 8,186 x 4 + 3,738 + 3,730 | 8,260
