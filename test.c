#include "hll.h"

int main()
{
        hll *testHll = HLL_INIT(65536); //Creates a hll structure with 65536 bytes of size

        int i;
        for(i = 0; i <= 10000000; i++)	//This loop will add 10^8 unique elements to the hll struct
        {
                HLL_ADD(&i,sizeof(i), testHll); //Adds an element to a hll struct
        }


        unsigned long card = HLL_CARDINALITY(testHll); //return the cardinality of the hll struct
        printf("This is the cardianlity: %lu\n",card);
}

