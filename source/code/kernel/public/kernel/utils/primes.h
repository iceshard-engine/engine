#pragma once
#include "kernel_pch.h"

typedef struct
{
    unsigned int *p;        /* pointer to array */
    int bitsPerByte;        /* 8 on normal systems */
    int bytesPerInt;        /* sizeof(unsigned int) */
    int bitsPerInt;            /* for bit arithmetic */
    int bpiShift;            /* 8 bit words = 3, 16=4, etc */
    int bpiMask;            /* bitsPerInch - 1, for modulus */
    uint64_t bitsInArray;        /* how many bits in array */
    uint64_t intsInArray;        /* how many uints to give necessary bits */
} BITARRAY;

void freeBitArray(BITARRAY* ba);
BITARRAY* createBitArray(uint64_t bits);
void setBit(BITARRAY* ba, uint64_t bitSS);
void clearBit(BITARRAY* ba, uint64_t bitSS);
int getBit(BITARRAY* ba, uint64_t bitSS);


void clearAll(BITARRAY* ba);
void setAll(BITARRAY* ba);
void printPrime(uint64_t bn);
uint64_t findPrime(uint64_t topCandidate);
