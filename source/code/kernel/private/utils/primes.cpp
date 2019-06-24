#include <kernel/utils/primes.h>
#include <cassert>

void freeBitArray(BITARRAY* ba)
{
    delete[] (ba->p);
    delete (ba);
}

BITARRAY* createBitArray(uint64_t bits)
{
    BITARRAY *ba = new BITARRAY;// static_cast<BITARRAY*>(MemNew(sizeof(BITARRAY)));
    assert(ba != NULL);
    ba->bitsPerByte = 8;
    ba->bytesPerInt = sizeof(unsigned int);
    ba->bitsPerInt = ba->bitsPerByte * ba->bytesPerInt;
    switch (ba->bitsPerInt)
    {
    case 8: ba->bpiShift = 3; break;
    case 16: ba->bpiShift = 4; break;
    case 32: ba->bpiShift = 5; break;
    case 64: ba->bpiShift = 6; break;
    case 128: ba->bpiShift = 7; break;
    case 256: ba->bpiShift = 8; break;
    case 512: ba->bpiShift = 9; break;
    default: {
            perror("ABORTING: Non-standard bits per int\n");
            exit(1);
            break;
        }
    };
    ba->bpiMask = ba->bitsPerInt - 1;
    ba->bitsInArray = bits;
    ba->intsInArray = bits / ba->bitsPerInt + 1;
    ba->p = (uint32_t*) new char[ba->intsInArray * ba->bytesPerInt];
    assert(ba->p != NULL);
    return ba;
}

void setBit(BITARRAY* ba, uint64_t bitSS)
{
    unsigned int *pInt = ba->p + (bitSS >> ba->bpiShift);
    unsigned int remainder = (bitSS & ba->bpiMask);
    *pInt |= (1 << remainder);
}

void clearBit(BITARRAY* ba, uint64_t bitSS)
{
    unsigned int *pInt = ba->p + (bitSS >> ba->bpiShift);
    unsigned int remainder = (bitSS & ba->bpiMask);
    unsigned int mask = 1 << remainder;
    mask = ~mask;
    *pInt &= mask;
}

int getBit(BITARRAY* ba, uint64_t bitSS)
{
    unsigned int *pInt = ba->p + (bitSS >> ba->bpiShift);
    unsigned int remainder = (bitSS & ba->bpiMask);
    unsigned int ret = *pInt;
    ret &= (1 << remainder);
    return(ret != 0);
}

void clearAll(BITARRAY* ba)
{
    uint64_t intSS;
    for (intSS = 0; intSS < ba->intsInArray; intSS++)
    {
        *(ba->p + intSS) = 0;
    }
}

void setAll(BITARRAY* ba)
{
    uint64_t intSS;
    for (intSS = 0; intSS < ba->intsInArray; intSS++)
    {
        *(ba->p + intSS) = ~0u;
    }
}

void printPrime(uint64_t bn)
{
    static char buf[1000];

    sprintf_s(buf, "%llu", bn);
    buf[strlen(buf) - 2] = '\0';
    printf("%s\n", buf);
}

uint64_t findPrime(uint64_t topCandidate)
{
    BITARRAY *ba = createBitArray(topCandidate);
    assert(ba != NULL);

    /* SET ALL BUT 0 AND 1 TO PRIME STATUS */
    setAll(ba);
    clearBit(ba, 0);
    clearBit(ba, 1);

    /* MARK ALL THE NON-PRIMES */
    uint64_t lastPrime = 1;
    uint64_t thisFactor = 2;
    uint64_t lastSquare = 0;
    uint64_t thisSquare = 0;
    while (thisFactor * thisFactor <= topCandidate)
    {
        /* MARK THE MULTIPLES OF THIS FACTOR */
        uint64_t mark = thisFactor + thisFactor;
        while (mark <= topCandidate)
        {
            clearBit(ba, mark);
            mark += thisFactor;
        }

        /* PRINT THE PROVEN PRIMES SO FAR */
        thisSquare = thisFactor * thisFactor;
        for (; lastSquare < thisSquare; lastSquare++)
        {
            if (getBit(ba, lastSquare)) lastPrime = lastSquare;
    //        if (getBit(ba, lastSquare)) printPrime(lastSquare);
        }

        /* SET thisFactor TO NEXT PRIME */
        thisFactor++;
        while (getBit(ba, thisFactor) == 0) thisFactor++;
        assert(thisFactor <= topCandidate);
    }

    /* PRINT THE REMAINING PRIMES */
    for (; lastSquare <= topCandidate; lastSquare++)
    {
        if (getBit(ba, lastSquare)) lastPrime = lastSquare;
    }
    freeBitArray(ba);
    return lastPrime;
}
