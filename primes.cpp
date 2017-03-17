#include <math.h>
#include <stdio.h>
#include <vector>



static bool isprime(unsigned long n, const std::vector<unsigned long> &priorPrimes)
{
    unsigned long max = floor(sqrt(n));

    for (std::vector<unsigned long>::const_iterator iter = priorPrimes.begin(); iter != priorPrimes.end(); iter++) {

        unsigned long test = *iter;

        if (!(n % test)) {
            return false;
        }

        if (test >= max) {
            break;
        }
    }

    return true;
}


int main(int argc, const char *argv[])
{
    if (2 != argc) {
        fprintf(stderr, "usage: %s <count>\n", argv[0]);
        return 1;
    }

    unsigned long count;
    if (1 != sscanf(argv[1], "%lu", &count)) {
        fprintf(stderr, "fatal: unsupported count: expected unsigned long but found \"%s\"\n", argv[1]);
        return 1;
    }

    if (!count) {
        return 0;
    }

    std::vector<unsigned long> priorPrimes;
    priorPrimes.push_back(2);

    unsigned long test = 3;
    while (count > 0) {
        if (isprime(test, priorPrimes)) {
            printf("%lu\n", test);
            priorPrimes.push_back(test);
            count--;
        }
        test += 2;
    }

    return 0;
}
