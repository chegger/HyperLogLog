#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <inttypes.h>

//Macros to use  for implementing hll in other c programs
#define HLL_INIT hllInit
#define HLL_ADD hllAdd
#define HLL_ADD_STR hllAdd_str
#define HLL_MERGE hllMerge
#define HLL_CARDINALITY getCardinality

typedef struct
{
        uint32_t        max_buckets;
        uint8_t         *buckets;
}hll;

uint8_t getMax(uint8_t,uint8_t);
uint32_t getBucketNumber(uint64_t);
uint8_t getTrailingZeroes(uint64_t);
double estimateFactor(unsigned, unsigned, double, double, double);
double mediumRangeCard(double);
uint64_t MurmurHash64A (const void*, int, unsigned int);
uint64_t stringHash(const char*);
uint32_t max_buckets;
uint64_t getCardinality(hll*);
hll *hllInit(uint32_t);
void hllAdd(const void*, int, hll*);
void hllAdd_str(const void*, int, hll*);
hll *hllMerge(hll*, hll*);

uint32_t max_buckets = 0;

hll *hllInit(uint32_t max)
{
	hll *newHll = calloc(1,sizeof(*newHll));
	newHll->buckets = calloc(1,65536 * sizeof(*(newHll->buckets)));
	newHll->max_buckets = max;
	return newHll;
}

void hllAdd(const void *data, int len, hll *currentHll)
{
	max_buckets = currentHll->max_buckets;
	uint32_t bucket;
	uint64_t hashedNumber = MurmurHash64A(data,len,0xadc83b19ULL);
	bucket = getBucketNumber(hashedNumber);

	currentHll->buckets[bucket] = getMax(currentHll->buckets[bucket], getTrailingZeroes(hashedNumber));
}

void hllAdd_str(const void *data, int len, hll *currentHll)
{

        max_buckets = currentHll->max_buckets;
        uint32_t bucket;
        uint64_t preHashedString = stringHash(data), *ptr = &preHashedString;
        uint64_t hashedNumber = MurmurHash64A(ptr,sizeof(*ptr),0xadc83b19ULL);
        bucket = getBucketNumber(hashedNumber);

        currentHll->buckets[bucket] = getMax(currentHll->buckets[bucket], getTrailingZeroes(hashedNumber));
}

hll *hllMerge(hll *hllA, hll *hllB)
{
	uint32_t memory = hllA->max_buckets;
	hll *hllResult = hllInit(memory);
	int i;
	for(i = 0; i<memory;i++)
		hllResult->buckets[i] = getMax(hllA->buckets[i], hllB->buckets[i]);
	return hllResult;
}

unsigned getBucketNumber(uint64_t hashedNumber)
{
    return (uint32_t)(hashedNumber & (max_buckets -1));
}

uint8_t getTrailingZeroes(uint64_t hashNumber)
{
        uint8_t ptr = 0;
        hashNumber = hashNumber >> (uint64_t)(log(max_buckets)/log(2));
        while((hashNumber & (1 << ptr)) == 0)
        {
                ptr++;
        }
        return ptr +1;
}

uint64_t getCardinality(hll *currentHll)
{
	max_buckets = currentHll->max_buckets;
	unsigned i,count = 0;
	double card,alpha = 0.7213,sum = 0;
	for (i = 0; i < max_buckets; i++)
	{
		if (currentHll->buckets[i])
		{
			sum += 1/pow(2,currentHll->buckets[i]);
			count++;
	        }
	}
	if(count == 0)
		return 0;

	//small cardinalities are estimated via linear count
        if(count <= max_buckets * 0.92)
        {
                unsigned negCount = max_buckets-count;
                int negMaxBuckets =  max_buckets * (-1);
                double v = (double)negCount/(double)max_buckets;
                card =  (double)negMaxBuckets * log(v);
                return card;
        }

	card = (pow(count,2) * alpha)/sum; //divide by sum = multiply with harmonic average

	//medium cardinalities need a small correction
	if(card <= max_buckets * 7.466)
		card = mediumRangeCard(card);

	//large cardinalities dont need any correction
	return (unsigned long)(card + 0.5);
}
double mediumRangeCard(double card)
{
					//This data was produced by experimenting
					//The following small factor corrections work with 16384 registers only
        unsigned mb = max_buckets;
                                                        //realCard for 14bit
        if (card <=(unsigned)( mb * 3.051))             //40k           7
                card  = card * 0.819;
        else if (card <= (unsigned)(mb * 3.236))        //45k           8
                card  = card * estimateFactor((unsigned)(mb*3.236), (unsigned)(mb*3.051), 0.843, 0.819, card);
        else if (card <= (unsigned)(mb*3.540))          //50k           9
                card  = card * estimateFactor((unsigned)(mb*3.540), (unsigned)(mb*3.236), 0.870, 0.843, card);
        else if (card <= (unsigned)(mb*3.784))          //55k           10
                card  = card * estimateFactor((unsigned)(mb*3.784), (unsigned)(mb*3.540), 0.895, 0.870, card);
        else if (card <= (unsigned)(mb*4.089))          //60k           11
                card  = card * estimateFactor((unsigned)(mb*4.089), (unsigned)(mb*3.784), 0.913, 0.895, card);
        else if (card <= (unsigned)(mb*4.333))          //65k           12
                card  = card * estimateFactor((unsigned)(mb*4.333), (unsigned)(mb*4.089), 0.930, 0.913, card);
        else if (card <= (unsigned)(mb*4.587))          //70k           13
                card  = card * estimateFactor((unsigned)(mb*4.578), (unsigned)(mb*4.333), 0.944, 0.930, card);
        else if (card <= (unsigned)(mb*4.822))          //75k           14
                card  = card * estimateFactor((unsigned)(mb*4.822), (unsigned)(mb*4.578), 0.954, 0.944, card);
        else if (card <= (unsigned)(mb*5.127))          //80k           15
                card  = card * estimateFactor((unsigned)(mb*5.127), (unsigned)(mb*4.822), 0.964, 0.954, card);
        else if (card <= (unsigned)(mb*5.402))          //85k           16
                card  = card * estimateFactor((unsigned)(mb*5.402), (unsigned)(mb*5.127), 0.972, 0.964, card);
        else if (card <= (unsigned)(mb*5.676))          //90k           17
                card  = card * estimateFactor((unsigned)(mb*5.676), (unsigned)(mb*5.402), 0.978, 0.972, card);
        else if (card <= (unsigned)(mb*6.012))          //95k           18
                card  = card * estimateFactor((unsigned)(mb*6.012), (unsigned)(mb*5.676), 0.981, 0.978, card);
        else if (card <= (unsigned)(mb*6.256))          //100k          19
                card  = card * estimateFactor((unsigned)(mb*6.256), (unsigned)(mb*6.012), 0.990, 0.981, card);
        else if (card <= (unsigned)(mb*6.561))          //105k          20
                card  = card * estimateFactor((unsigned)(mb*6.561), (unsigned)(mb*6.256), 0.992, 0.990, card);
        else if (card <= (unsigned)(mb*6.805))          //110k          21
                card  = card * estimateFactor((unsigned)(mb*6.805), (unsigned)(mb*6.561), 0.992, 0.992, card);
        else if (card <= (unsigned)(mb*7.141))          //115k          22
                card  = card * estimateFactor((unsigned)(mb*7.141), (unsigned)(mb*6.805), 0.992, 0.992, card);
        else if (card <= (unsigned)(mb*7.446))          //120k          23
                card  = card * estimateFactor((unsigned)(mb*7.446), (unsigned)(mb*7.141), 0.998, 0.992, card);

        return card;
}

double estimateFactor(unsigned upper, unsigned lower, double upper_percent, double lower_percent, double card)
{
	unsigned diff_upper_lower, diff_card_lower;
	double diff_percent,diff_relative,factor;

        diff_upper_lower = upper-lower;
        diff_card_lower = card - lower;
        diff_percent =  upper_percent-lower_percent;
        diff_relative = (double)(diff_card_lower)/(double)(diff_upper_lower);

        factor = lower_percent + (diff_percent * diff_relative);
	return factor;
}

uint8_t getMax(uint8_t a, uint8_t b)
{
    return a > b ? a : b;
}


/****************** This is the hashfunction used ******************/
uint64_t MurmurHash64A (const void * key, int len, unsigned int seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;
    uint64_t h = seed ^ (len * m);
    const uint8_t *data = (const uint8_t *)key;
    const uint8_t *end = data + (len-(len&7));
    while(data != end) {
        uint64_t k;

        k = (uint64_t) data[0];
        k |= (uint64_t) data[1] << 8;
        k |= (uint64_t) data[2] << 16;
        k |= (uint64_t) data[3] << 24;
        k |= (uint64_t) data[4] << 32;
        k |= (uint64_t) data[5] << 40;
        k |= (uint64_t) data[6] << 48;
        k |= (uint64_t) data[7] << 56;

        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;
        data += 8;
    }

    switch(len & 7) {
    case 7: h ^= (uint64_t)data[6] << 48;
    case 6: h ^= (uint64_t)data[5] << 40;
    case 5: h ^= (uint64_t)data[4] << 32;
    case 4: h ^= (uint64_t)data[3] << 24;
    case 3: h ^= (uint64_t)data[2] << 16;
    case 2: h ^= (uint64_t)data[1] << 8;
    case 1: h ^= (uint64_t)data[0];
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}
uint64_t stringHash(const char *str)
{
        unsigned long hash = 5381;
        int c;
        while ((c = *str++))
                hash = ((hash << 5) + hash) + c;
        return hash;
}

