# multper
Efficient Multiplicative Persistence checker

Inspired by the NumberPhile video about 277777788888899
https://www.youtube.com/watch?v=Wim9WJeDTHQ

This program represents large numbers as a "bag" of digits, where
it only needs to store the count of each digit [0:9] in the overall number.

Multiplying digits is done by taking each digit to the power of its 
given count, and multiplying the resulting powers together.

Powers are calculated up to the max specified length and 
cached ahead of time before doing any checks.  This makes each 
persistence check boil down to a few quick table lookups followed by a few 
multiplications, depending on which digits are present.

It also iterates selectively over which numbers to check based on some specific rules which I found from: https://oeis.org/A003001

One of the statements in the video was that numbers had been checked up to 233 digits long.
It only takes a few seconds to check up to 233 digits using this approach.

## Dependencies
 - Boost
 - GMP library
 - compiler supporting C++17

## Compiling
Just run:
```
g++ -O3 -o multper ./multiplicative_persistence.cc -lgmp -std=c++17
```
OR
```
clang++ -O3 -o multper ./multiplicative_persistence.cc -lgmp -std=c++17
```
