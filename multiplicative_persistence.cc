#include <iostream>
//#include <unordered_map>
#include <vector>
#include <algorithm>

// Check for alternative Multiprecision Implementation from boost
#if defined(CPP_INT)
    // alternative cpp_int (~4x slower than GMP)
    // no external linking required
    // use compile command with "-DCPP_INT"
    #include <boost/multiprecision/cpp_int.hpp>
    using boost_mp_int = boost::multiprecision::cpp_int;
#elif defined(TOM_INT)
    // alternative libtommath (~50x slower than GMP)
    // use compile command with "-DTOM_INT -ltommath"
    #include <boost/multiprecision/tommath.hpp>
    using boost_mp_int = boost::multiprecision::tom_int;
#else
    // Default GMP (fastest)
    // use compile command with "-lgmp"
    #include <boost/multiprecision/gmp.hpp>
    using boost_mp_int = boost::multiprecision::mpz_int;
#endif

#include <limits>
#include <cstdlib>
#include <cassert>
using namespace std;
using namespace boost::multiprecision;

// Efficient Multiplicative Persistence checker by Hans Loeblich
// This represents large numbers as a collection of digit counts
// for [0:9]
// Multiplying digits is done by taking each digit to the power of its
// given count, and multiplying the resulting powers together.
//
// Powers are calculated and cached up to the max specified lengths
// before doing any checks.  This makes each persistence check
// boil down to some fast lookups of powers, and then a few
// multiplications depending on which digits are present.
//
// Inspired by the NumberPhile video about 277777788888899
// https://www.youtube.com/watch?v=Wim9WJeDTHQ

class digit_bag {
public:
    typedef array<vector<boost_mp_int>, 10> pow_cache_t;
    typedef vector<size_t> rule_prefix_t; // the prefix for a rule
    typedef vector<size_t> rule_tail_digits_t; // list of possible digits for tail
    typedef pair<rule_prefix_t,rule_tail_digits_t> rule_t;
    typedef vector<rule_t> rules_t;
    typedef array<size_t, 10> digits_t;

    digit_bag(size_t length) : size(length), rule_it(rules.begin()), digits(), result_str(length+1,'\0') {
        init_rule(*rule_it);
    }
    digit_bag(digit_bag&&) = delete;
    digit_bag& operator=(digit_bag&&) = delete;	
    digit_bag(const digit_bag&) = delete;
    digit_bag& operator=(const digit_bag&) = delete;

    // use a fast lookup table for powers of each digit up to max length
    // ignore 0 and 1
    static void init_cache(size_t max) {
        boost_mp_int power;
        for(size_t i = 2; i < 10; ++i) {
            boost_mp_int base = i;
            for(size_t p = 0; p <= max; ++p) {
                power = pow(base, p);
                cache[i].push_back(power);
            }
        }
    }

    // when iterating to a new rule,
    // initialize digits based on lowest number in the rule
    inline void init_rule(const rule_t& rule) {
        // reset digit counts
        for(auto &d : digits) d = 0;
        // initialize head
        for(const size_t di : rule.first) digits[di]++;
        // set remaining digits to first option in tail_digits
        digits[rule.second[0]] += size - rule.first.size();
    }

    // check every combination of given length
    // starting from digits already in bag
    pair<digits_t, size_t> check_all() {
        digits_t current_max;
        size_t current_max_count = 0;
        do {
            size_t count = persistence();
            if (count > current_max_count) {
                current_max_count = count;
                current_max = digits;
            }
        } while(next());
        return make_pair(current_max,current_max_count);
    }

    // check persitence of current digits
    // assumes input is already length > 1
    size_t persistence() const {
        result = 1;
        for (size_t i=2; i<10; ++i) {
            if (digits[i]) {
                result *= cache[i][digits[i]];
            }
        }
        size_t mult_count = 1;

        while (true) {
            digits_t new_digits{0,0,0,0,0,0,0,0,0,0};
            constexpr const size_t CH_ZERO = '0';
            // convert result to string and count digits of each type
        #if !defined(CPP_INT) && !defined(TOM_INT)
            // Minor optimization (~2%) for GMP string conversion
            mpz_get_str(&result_str[0], 10, result.backend().data());
            if (result_str[1] == '\0') { // single digit result
                return mult_count;
            }
            for(size_t ch : result_str) {
                if (ch == '\0') break;
                size_t di = ch - CH_ZERO; // turn character into numeric digit
                if (di == 0) {
                    return mult_count+1;
                }
                ++new_digits[di];
            }
        #else
            // Generic std::string check
            result_str = result.str();
            if (result_str.size() == 1) { // single digit result
                return mult_count;
            }
            for(size_t ch : result_str) {
                size_t di = ch-'0'; // turn character into numeric digit
                if (di == 0) {
                    return mult_count+1;
                }
                ++new_digits[di];
            }
        #endif
            // we can't check this inside the loop in case there is a zero also,
            // we would return 1 higher than expected
            if (new_digits[5] && (new_digits[2] || new_digits[4] || new_digits[6] || new_digits[8])) {
                return mult_count+2;
            }

            result = 1;
            for (size_t i=2; i<10; ++i) {
                if (new_digits[i]) {
                    result *= cache[i][new_digits[i]];
                }
            }
            ++mult_count;
        }
    }

    // get the next set of digits, in ascending order based on rules
    inline bool next() noexcept {
        const rule_prefix_t &head = rule_it->first;
        const rule_tail_digits_t &tail_digits = rule_it->second;
        const size_t last_digit = tail_digits[tail_digits.size()-1];
        if (digits[last_digit] == size - head.size()) {
            rule_it++;
            if (rule_it == rules.end()) return false;
            init_rule(*rule_it);
            return true;
        } else {
            const size_t tds = tail_digits.size();
            if (tds >= 2) {
                for(size_t ti=tds-2; ti>=0; --ti) {
                    const size_t di = tail_digits[ti];
                    if (digits[di]) {
                        const size_t di2 = tail_digits[ti+1];
                        digits[di]--;
                        digits[di2]++;
                        for(size_t ji2=ti+2; ji2<tds; ++ji2) {
                            const size_t di3 = tail_digits[ji2];
                            digits[di2] += digits[di3];
                            digits[di3] = 0;
                        }
                        return true;
                    }
                }
                //assert(false && "where the digits go?");
            } else {
                rule_it++;
                if (rule_it == rules.end()) return false;
                init_rule(*rule_it);
                return true;
            }
        }
    }

    inline const digits_t& getDigits() const {
        return digits;
    }

private:
    static const rules_t rules;
    static pow_cache_t cache;

    size_t size;
    rules_t::const_iterator rule_it;
    digits_t digits;
    mutable string result_str;
    mutable boost_mp_int result;
};

ostream& operator<<(ostream &out, const digit_bag::digits_t &digits)
{
    out << "{ " << digits[0];
    for (size_t i = 1; i<10; ++i) cout << ", " << digits[i];
    cout << " }";
    return out;
}

// https://oeis.org/A003001
// Summarizing, a term a(n) for n > 2 consists of 7's, 8's and 9's
// with a prefix of one of the following sets of digits:
// {{}, {2}, {3}, {4}, {6}, {2,6}, {3,5}, {5, 5,...}}
// [Amended by Kohei Sakai, May 27 2017]

// Rules are listed to generate numbers in ascending order
const digit_bag::rules_t digit_bag::rules{
    {{2,6},{7,8,9}},
    {{2},  {7,8,9}},
    {{3,5},{7,8,9}},
    {{3},  {7,8,9}},
    {{4},  {7,8,9}},
    {{5},  {5,7,9}},
    {{6},  {7,8,9}},
    {{ },  {7,8,9}}
};
digit_bag::pow_cache_t digit_bag::cache;



int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        cout << "Search for the smallest numbers with the highest multiplicative persistence for base10 numbers with lengths in the range of [START:END)\n";
        cout << "   Or in other words: START < log10(number) < END\n";
        cout << "usage: multper START END [MAX]\n";
        cout << "   START   range lower bound(inclusive).  START must be >= 2\n";
        cout << "   END     range upper bound(exclusive)\n";
        cout << "   MAX     (optional) sets the maximum persistence to treat specially(default=0 regardless of START length)\n";
        cout << "     Any number found with persistence greater than MAX will print a line starting with \"NEW MAX\", with empty lines before/after for visibility.\n";
        cout << "     Otherwise print a single line for each length indicating the best match (smallest number with highest persistence for that length)\n";
        return 1;
    }

    const size_t START = stoul(argv[1]);
    assert(START>=2 && "");
    const size_t END = stoul(argv[2]);
    size_t max_count = 0;
    if (argc == 4) {
        max_count = stoul(argv[3]);
    }

    //cout << "Initializing lookup..."; cout.flush();
    digit_bag::init_cache(END);
    //cout << "Done." << endl;

    for(size_t length=START; length<END; ++length) {
        digit_bag bag(length);
        pair<digit_bag::digits_t, size_t> current_max = bag.check_all();
        if (current_max.second > max_count) {
            max_count = current_max.second;
            cout << endl << "NEW MAX " << max_count << " persistence for " << length << " digits: " << current_max.first << endl << endl;
        } else {
            cout << current_max.second << " best persistence for " << length << " digits: " << current_max.first << endl;
        }
    }
    return 0;
}
