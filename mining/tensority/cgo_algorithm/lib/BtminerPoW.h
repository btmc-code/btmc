/* BtminerPoW.h */
#ifndef BtminerPOW_H
#define BtminerPOW_H

#include "scrypt.h"
#include "sha3-allInOne.h"
#include <iostream>
#include <vector>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <x86intrin.h>
#include <omp.h>

#define FNV(v1,v2) int32_t( ((v1)*FNV_PRIME) ^ (v2) )
const int FNV_PRIME = 0x01000193;

struct Mat256x256i8 {
    int8_t d[256][256];

    void toIdentityMatrix() {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                d[i][j] = (i==j)?1:0; // diagonal
            }
        }
    }

    void copyFrom(const Mat256x256i8& other) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                this->d[j][i] = other.d[j][i];
            }
        }
    }

    Mat256x256i8() {
//        this->toIdentityMatrix();
    }

    Mat256x256i8(const Mat256x256i8& other) {
        this->copyFrom(other);
    }

    void copyFrom_helper(LTCMemory& ltcMem, int offset) {
        for(int i = 0; i < 256; i++) {
            const Words32& lo=ltcMem.get(i*4 + offset);
            const Words32& hi=ltcMem.get(i*4 + 2 + offset);
            for(int j = 0; j < 64; j++) {
                uint32_t i32 = j>=32?hi.get(j-32):lo.get(j);
                d[j*4+0][i] = (i32>> 0) & 0xFF;
                d[j*4+1][i] = (i32>> 8) & 0xFF;
                d[j*4+2][i] = (i32>>16) & 0xFF;
                d[j*4+3][i] = (i32>>24) & 0xFF;
            }
        }
    }

    void copyFromEven(LTCMemory& ltcMem) {
        copyFrom_helper(ltcMem, 0);
    }

    void copyFromOdd(LTCMemory& ltcMem) {
        copyFrom_helper(ltcMem, 1);
    }

    void add(Mat256x256i8& a, Mat256x256i8& b) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                int tmp = int(a.d[i][j]) + int(b.d[i][j]);
                this->d[i][j] = (tmp & 0xFF);
            }
        }
    }
};

struct Mat256x256i16 {
    int16_t d[256][256];

    void toIdentityMatrix() {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                d[i][j] = (i==j?1:0); // diagonal
            }
        }
    }

    void copyFrom(const Mat256x256i8& other) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                this->d[j][i] = int16_t(other.d[j][i]);
                assert(this->d[j][i] == other.d[j][i]);
            }
        }
    }

    void copyFrom(const Mat256x256i16& other) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                this->d[j][i] = other.d[j][i];
            }
        }
    }

    Mat256x256i16() {
//        this->toIdentityMatrix();
    }

    Mat256x256i16(const Mat256x256i16& other) {
        this->copyFrom(other);
    }

    void copyFrom_helper(LTCMemory& ltcMem, int offset) {
        for(int i = 0; i < 256; i++) {
            const Words32& lo = ltcMem.get(i*4 + offset);
            const Words32& hi = ltcMem.get(i*4 + 2 + offset);
            for(int j = 0; j < 64; j++) {
                uint32_t i32 = j>=32?hi.get(j-32):lo.get(j);
                d[j*4+0][i] = int8_t((i32>> 0) & 0xFF);
                d[j*4+1][i] = int8_t((i32>> 8) & 0xFF);
                d[j*4+2][i] = int8_t((i32>>16) & 0xFF);
                d[j*4+3][i] = int8_t((i32>>24) & 0xFF);
            }
        }
    }

    void copyFromEven(LTCMemory& ltcMem) {
        copyFrom_helper(ltcMem, 0);
    }

    void copyFromOdd(LTCMemory& ltcMem) {
        copyFrom_helper(ltcMem, 1);
    }

    void mul(const Mat256x256i16& a, const Mat256x256i16& b) {
        for(int i = 0; i < 256; i += 16) {
            for(int j = 0; j < 256; j += 16) {
                for(int ii = i; ii < i+16; ii += 8) {
                    __m256i r[8],s,t[8],u[8],m[8];
                    r[0] = _mm256_set1_epi16(0);
                    r[1] = _mm256_set1_epi16(0);
                    r[2] = _mm256_set1_epi16(0);
                    r[3] = _mm256_set1_epi16(0);
                    r[4] = _mm256_set1_epi16(0);
                    r[5] = _mm256_set1_epi16(0);
                    r[6] = _mm256_set1_epi16(0);
                    r[7] = _mm256_set1_epi16(0);
                    for(int k = 0; k < 256; k++) {
                        s = *((__m256i*)(&(b.d[k][j])));
                        u[0] = _mm256_set1_epi16(a.d[ii+0][k]);
                        u[1] = _mm256_set1_epi16(a.d[ii+1][k]);
                        u[2] = _mm256_set1_epi16(a.d[ii+2][k]);
                        u[3] = _mm256_set1_epi16(a.d[ii+3][k]);
                        u[4] = _mm256_set1_epi16(a.d[ii+4][k]);
                        u[5] = _mm256_set1_epi16(a.d[ii+5][k]);
                        u[6] = _mm256_set1_epi16(a.d[ii+6][k]);
                        u[7] = _mm256_set1_epi16(a.d[ii+7][k]);
                        m[0] = _mm256_mullo_epi16(u[0],s);
                        m[1] = _mm256_mullo_epi16(u[1],s);
                        m[2] = _mm256_mullo_epi16(u[2],s);
                        m[3] = _mm256_mullo_epi16(u[3],s);
                        m[4] = _mm256_mullo_epi16(u[4],s);
                        m[5] = _mm256_mullo_epi16(u[5],s);
                        m[6] = _mm256_mullo_epi16(u[6],s);
                        m[7] = _mm256_mullo_epi16(u[7],s);
                        r[0] = _mm256_add_epi16(r[0],m[0]);
                        r[1] = _mm256_add_epi16(r[1],m[1]);
                        r[2] = _mm256_add_epi16(r[2],m[2]);
                        r[3] = _mm256_add_epi16(r[3],m[3]);
                        r[4] = _mm256_add_epi16(r[4],m[4]);
                        r[5] = _mm256_add_epi16(r[5],m[5]);
                        r[6] = _mm256_add_epi16(r[6],m[6]);
                        r[7] = _mm256_add_epi16(r[7],m[7]);
                    }
                    t[0] = _mm256_slli_epi16(r[0],8);
                    t[1] = _mm256_slli_epi16(r[1],8);
                    t[2] = _mm256_slli_epi16(r[2],8);
                    t[3] = _mm256_slli_epi16(r[3],8);
                    t[4] = _mm256_slli_epi16(r[4],8);
                    t[5] = _mm256_slli_epi16(r[5],8);
                    t[6] = _mm256_slli_epi16(r[6],8);
                    t[7] = _mm256_slli_epi16(r[7],8);
                    t[0] = _mm256_add_epi16(r[0],t[0]);
                    t[1] = _mm256_add_epi16(r[1],t[1]);
                    t[2] = _mm256_add_epi16(r[2],t[2]);
                    t[3] = _mm256_add_epi16(r[3],t[3]);
                    t[4] = _mm256_add_epi16(r[4],t[4]);
                    t[5] = _mm256_add_epi16(r[5],t[5]);
                    t[6] = _mm256_add_epi16(r[6],t[6]);
                    t[7] = _mm256_add_epi16(r[7],t[7]);
                    for(int x = 0; x < 8; x++) {
                        this->d[ii+x][j+0 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*0 +1)));
                        this->d[ii+x][j+1 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*1 +1)));
                        this->d[ii+x][j+2 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*2 +1)));
                        this->d[ii+x][j+3 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*3 +1)));
                        this->d[ii+x][j+4 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*4 +1)));
                        this->d[ii+x][j+5 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*5 +1)));
                        this->d[ii+x][j+6 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*6 +1)));
                        this->d[ii+x][j+7 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*7 +1)));
                        this->d[ii+x][j+8 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*8 +1)));
                        this->d[ii+x][j+9 ] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*9 +1)));
                        this->d[ii+x][j+10] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*10+1)));
                        this->d[ii+x][j+11] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*11+1)));
                        this->d[ii+x][j+12] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*12+1)));
                        this->d[ii+x][j+13] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*13+1)));
                        this->d[ii+x][j+14] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*14+1)));
                        this->d[ii+x][j+15] = int16_t(int8_t(_mm256_extract_epi8(t[x],2*15+1)));
                    }
                }
            }
        }
    }

    void add(Mat256x256i16& a, Mat256x256i16& b) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                int tmp = int(a.d[i][j]) + int(b.d[i][j]);
                this->d[i][j] = (tmp & 0xFF);
            }
        }
    }

    void toMatI8(Mat256x256i8& other) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                other.d[j][i] = (this->d[j][i]) & 0xFF;
            }
        }
    }

    void topup(Mat256x256i8& other) {
        for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                other.d[j][i] += (this->d[j][i]) & 0xFF;
            }
        }
    }
};


struct Arr256x64i32 {
    uint32_t d[256][64];

    uint8_t* d0RawPtr() {
        return (uint8_t*)(d[0]);
    }

    Arr256x64i32(const Mat256x256i8& mat) {
        for(int j = 0; j < 256; j++) {
            for(int i = 0; i < 64; i++) {
                d[j][i] = ((uint32_t(uint8_t(mat.d[j][i + 192]))) << 24) |
                          ((uint32_t(uint8_t(mat.d[j][i + 128]))) << 16) |
                          ((uint32_t(uint8_t(mat.d[j][i +  64]))) <<  8) |
                          ((uint32_t(uint8_t(mat.d[j][i]))) << 0);
            }
        }
    }

    void reduceFNV() {
        for(int k = 256; k > 1; k = k/2) {
            for(int j = 0; j < k/2; j++) {
                for(int i = 0; i < 64; i++) {
                    d[j][i] = FNV(d[j][i], d[j + k/2][i]);
                }
            }
        }
    }
};


#endif

