#include <iostream>
//#include <unordered_map>
#include <vector>
#include <algorithm>
#include <boost/multiprecision/gmp.hpp>
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
    typedef array<vector<mpz_int>, 10> pow_cache_t;
    typedef vector<unsigned char> rule_prefix_t; // the prefix for a rule
    typedef vector<unsigned char> rule_tail_digits_t; // list of possible digits for tail 
    typedef pair<rule_prefix_t,rule_tail_digits_t> rule_t;
    typedef vector<rule_t> rules_t;
    typedef array<unsigned int, 10> digits_t; 

    digit_bag(size_t length) : size(length), rule_it(rules.begin()), digits(), result_str(length+1,'\0') { 
        init_rule(*rule_it); 
    }
    digit_bag(digit_bag&&) = delete;
    digit_bag& operator=(digit_bag&&) = delete;	
    digit_bag(const digit_bag&) = delete;
    digit_bag& operator=(const digit_bag&) = delete;

    // use a fast lookup table for powers of each digit up to max length
    // ignore 0 and 1
    static void init_cache(unsigned long max) {
        mpz_t power;
        mpz_init(power);
        for(int i = 2; i < 10; ++i) {
            for(unsigned int p = 0; p <= max; ++p) {
                mpz_ui_pow_ui(power, i, p);
                cache[i].push_back(power);
            }
        }
        mpz_clear(power);
    }

    // when iterating to a new rule, 
    // initialize digits based on lowest number in the rule
    inline void init_rule(const rule_t& rule) {
        // reset digit counts
        for(auto &d : digits) d = 0; 
        // initialize head
        for(const auto di : rule.first) digits[di]++; 
        // set remaining digits to first option in tail_digits
        digits[rule.second[0]] += size - rule.first.size(); 
    }

    // check every combination of given length 
    // starting from digits already in bag
    pair<digits_t, unsigned int> check_all() {
        digits_t current_max;
        unsigned int current_max_count = 0;
        do {
            unsigned int count = persistence();
            if (count > current_max_count) {
                current_max_count = count;
                current_max = digits;
            }
        } while(next());
        return make_pair(current_max,current_max_count);
    }

    // check persitence of current digits
    // assumes input is already length > 1
    unsigned int persistence() const {
        result = 1;
        for (unsigned i=2; i<10; ++i) {
            if (digits[i]) {
                result *= cache[i][digits[i]];
            }
        }
        int mult_count = 1;

        while (true) {
            digits_t new_digits{0,0,0,0,0,0,0,0,0,0};
            // convert result to string and count digits of each type
            mpz_get_str(result_str.data(), 10, result.backend().data());
            if (result_str[1] == '\0') { // single digit result
                return mult_count;
            }
            for(auto ch : result_str) {
                if (ch == '\0') break;
                int di = ch-'0'; // turn character into numeric digit
                if (di == 0) {
                    return mult_count+1;
                }
                ++new_digits[di];
            }
            // we can't check this inside the loop in case there is a zero also, 
            // we would return 1 higher than expected
            if (new_digits[5] && (new_digits[2] || new_digits[4] || new_digits[6] || new_digits[8])) {
                return mult_count+2;
            }

            result = 1;
            for (unsigned i=2; i<10; ++i) {
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
        const auto last_digit = tail_digits[tail_digits.size()-1];
        if (digits[last_digit] == size - head.size()) {
            rule_it++;
            if (rule_it == rules.end()) return false;
            init_rule(*rule_it);
            return true;
        } else {
            const auto tds = tail_digits.size();
            if (tds >= 2) {
                for(auto ti=tds-2; ti>=0; --ti) {
                    const auto di = tail_digits[ti];
                    if (digits[di]) {
                        const auto di2 = tail_digits[ti+1];
                        digits[di]--;
                        digits[di2]++;
                        for(auto ji2=ti+2; ji2<tds; ++ji2) {
                            const auto di3 = tail_digits[ji2];
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

    inline void tostream(ostream& out) const {
        stream_digits(out, digits);
    }

    inline static void stream_digits(ostream& out, const digits_t& digits) {
        out << "{ " << digits[0];
        for (unsigned int i = 1; i<10; ++i) cout << ", " << digits[i];
            cout << " }" << endl;
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
    mutable mpz_int result;
};

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
    if (argc != 3) {
        cout << "Search for multiplicative persistence of numbers with length in the range of [START:END)" << endl;
        cout << "usage: multper START END" << endl;
        return 1;
    }
    const unsigned long START = stoul(argv[1]);
    const unsigned long END = stoul(argv[2]);
    assert(START>=2);

    cout << "initializing lookup..."; cout.flush();
    digit_bag::init_cache(END);
    cout << "Done." << endl;

    unsigned int max_count = 0;
    for(unsigned int length=START; length<END; ++length) {
        digit_bag bag(length);
        pair<digit_bag::digits_t, unsigned int> current_max = bag.check_all();
        if (current_max.second > max_count) {
            max_count = current_max.second;
            cout << "\nNEW MAX " << max_count << " persistence for " << length << " digits: ";
            digit_bag::stream_digits(cout, current_max.first);
            cout << endl;
        } else {
            cout << current_max.second << " best persistence for " << length << " digits: ";
            digit_bag::stream_digits(cout, current_max.first);
        }
    }
    return 0;
}


