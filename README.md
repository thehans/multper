# multper
Efficient Multiplicative Persistence checker

Inspired by the Numberphile video about 277777788888899
https://www.youtube.com/watch?v=Wim9WJeDTHQ

This program represents large numbers as a "bag" of digits, where
it only needs to store number of times each digit [0:9] occurs in the overall number.

Multiplying digits is done by taking each digit to the power of its 
given count, and multiplying the resulting powers together.

Powers are calculated up to the max specified length and 
cached ahead of time before doing any checks.  This makes each 
persistence check boil down to a few quick table lookups followed by a few 
multiplications, depending on which digits are present.

It also iterates selectively over which numbers to check based on some specific rules which I found from: https://oeis.org/A003001
> term a(n) for n > 2 consists of 7's, 8's and 9's with a prefix of one of the following sets of digits: {{}, {2}, {3}, {4}, {6}, {2,6}, {3,5}, {5, 5,...}}

One of the statements in the video was that numbers had been checked up to 233 digits long.
Numbers from 2 to 233 digits are computed in 11s on my laptop using this approach.

## Usage
```
multper START END [MAX]
```
Where `START >= 2`, will check all numbers (which match the above rules) with a number of digits in the range of `[START:END)`.  For each length checked, the smallest number of that length with the highest persistence is printed.
Optional `MAX` argument indicates the initial value when checking maximum persistence, above which a special `NEW MAX` output line will be printed.  (Default 0)

So far I have tried checking up to around 1400 digits, but unfortunately it seems like the max persistence drops to 2 after a certain length. I'm fairly certain the algorithm for checking and generation the next combination based on rules is all correct, but welcome any bug reports or suggestions for enhancement.

The program is single-threaded, but can be run on multiple cores by calling multiple instances with different length ranges.  The end number is not included in the search for simplifying running batches like this.  So for example you can run `multper 1000 1100` and `multper 1100 1200` without the ranges overlapping.  

I have added a bash script `do_work.sh` which takes 4 arguments representing `START` `STEP` `END` and `CPUS`, and evaluates from `START` to `END`, splitting the load across `CPUS` #of parallel tasks, with each task taking a range of digits equal to `STEP`.

Let me know if you find any persistence higher than 11 with my program!

## Dependencies
 - [Boost Multiprecision](https://www.boost.org/doc/libs/1_69_0/libs/multiprecision/doc/html/index.html)
    Boost Multiprecision supports three backends, GMP, libtommath, and cpp_int

 ### Optional Dependencies
 - [GMP](https://gmplib.org/) (technically optional as boost offers other multiprecision implementations, but **Highly Recommended** for fastest execution)
 - [libtommath](https://github.com/libtom/libtommath) Alternative multiprecision implementation (Warning: ~50x slower than GMP)
 - "cpp_int" is a multiprecision implementation builtin to boost which can be used in lieu of the above libraries for no added dependency. (Warning: ~4x slower than GMP) 

## Compiling
`multper` can be built using `cmake`, just define which multiprecision library to use with `-DMPLIB=` `GMP`, `CPP`, or `TOM`
Example:
```
cmake -DMPLIB=GMP
make
```

## Example output

```
./multper 2 18

NEW MAX 4 persistence for 2 digits: { 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 }


NEW MAX 5 persistence for 3 digits: { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1 }


NEW MAX 6 persistence for 4 digits: { 0, 0, 0, 0, 0, 0, 1, 1, 2, 0 }


NEW MAX 7 persistence for 5 digits: { 0, 0, 0, 0, 0, 0, 1, 0, 3, 1 }

7 best persistence for 6 digits: { 0, 0, 1, 0, 0, 0, 1, 2, 0, 2 }

NEW MAX 8 persistence for 7 digits: { 0, 0, 1, 0, 0, 0, 1, 2, 2, 1 }


NEW MAX 9 persistence for 8 digits: { 0, 0, 1, 0, 0, 0, 1, 0, 3, 3 }

7 best persistence for 9 digits: { 0, 0, 0, 0, 0, 0, 0, 0, 9, 0 }

NEW MAX 10 persistence for 10 digits: { 0, 0, 0, 1, 0, 0, 0, 2, 4, 3 }

7 best persistence for 11 digits: { 0, 0, 1, 0, 0, 0, 1, 6, 1, 2 }
9 best persistence for 12 digits: { 0, 0, 0, 1, 0, 0, 0, 3, 7, 1 }
10 best persistence for 13 digits: { 0, 0, 0, 1, 0, 0, 0, 0, 11, 1 }
9 best persistence for 14 digits: { 0, 0, 1, 0, 0, 0, 0, 12, 0, 1 }

NEW MAX 11 persistence for 15 digits: { 0, 0, 1, 0, 0, 0, 0, 6, 6, 2 }

6 best persistence for 16 digits: { 0, 0, 1, 0, 0, 0, 1, 10, 4, 0 }
11 best persistence for 17 digits: { 0, 0, 1, 0, 0, 0, 0, 5, 1, 10 }
```

As you can see in the output, the numbers between braces are the count of each digit from 0 to 9.
```
NEW MAX 11 persistence for 15 digits: { 0, 0, 1, 0, 0, 0, 0, 6, 6, 2 }
```
Which corresponds to: `277777788888899`

