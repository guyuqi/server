/*
   Copyright (c) 2000, 2012, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING. If not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
   MA  02110-1301  USA.
*/

/* aes.hpp defines AES
*/


#ifndef TAO_CRYPT_AES_HPP
#define TAO_CRYPT_AES_HPP

#include <stdio.h>
#include "misc.hpp"
#include "modes.hpp"

#if defined(TAOCRYPT_X86ASM_AVAILABLE) && defined(TAO_ASM)
    #define DO_AES_ASM
#endif

namespace TaoCrypt {

#ifdef HAVE_ARMV8_CRC_CRYPTO_INTRINSICS
    #include <arm_neon.h>
    #define EXPND_R_KEY_MSIZE 60*4

    #if defined(__BYTE_ORDER__)
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            #define LITTLE_ENDIAN_ORDER
        #else
            #define BIG_ENDIAN_ORDER
        #endif
    #else
        #error macro __BYTE_ORDER__ is not defined in host compiler
    #endif
   //#define ARMV8_CE_DEBUG
#endif

enum { AES_BLOCK_SIZE = 16 };


// AES encryption and decryption, see FIPS-197
class AES : public Mode_BASE {
public:
    enum { BLOCK_SIZE = AES_BLOCK_SIZE };

    AES(CipherDir DIR, Mode MODE)
        : Mode_BASE(BLOCK_SIZE, DIR, MODE) {}

#ifdef DO_AES_ASM
    void Process(byte*, const byte*, word32);
#endif
#ifdef HAVE_ARMV8_CRC_CRYPTO_INTRINSICS
    void Process(byte*, const byte*, word32);
    void armv8_aes_blkcrypt(int, uint8x16_t*, const uint8x16_t*);
#endif
    void SetKey(const byte* key, word32 sz, CipherDir fake = ENCRYPTION);
    void SetIV(const byte* iv) { memcpy(r_, iv, BLOCK_SIZE); }
private:
    static const word32 rcon_[];

    word32      rounds_;
    word32      key_[60];                        // max size

    static const word32 Te[5][256];
    static const word32 Td[5][256];
    static const byte   CTd4[256];

    static const word32* Te0;
    static const word32* Te1;
    static const word32* Te2;
    static const word32* Te3;
    static const word32* Te4;

    static const word32* Td0;
    static const word32* Td1;
    static const word32* Td2;
    static const word32* Td3;
    static const word32* Td4;

    void encrypt(const byte*, const byte*, byte*) const;
    void AsmEncrypt(const byte*, byte*, void*) const;
    void decrypt(const byte*, const byte*, byte*) const;
    void AsmDecrypt(const byte*, byte*, void*) const;

    void ProcessAndXorBlock(const byte*, const byte*, byte*) const;

    word32 PreFetchTe() const;
    word32 PreFetchTd() const;
    word32 PreFetchCTd4() const;

    AES(const AES&);            // hide copy
    AES& operator=(const AES&); // and assign
};


#if defined(__x86_64__) || defined(_M_X64) || \
           (defined(__ILP32__) && (__ILP32__ >= 1))
    #define TC_CACHE_LINE_SZ 64
#else
    /* default cache line size */
    #define TC_CACHE_LINE_SZ 32
#endif

inline word32 AES::PreFetchTe() const
{
    word32 x = 0;

    /* 4 tables of 256 entries */
    for (int i = 0; i < 4; i++) {
        /* each entry is 4 bytes */
        for (int j = 0; j < 256; j += TC_CACHE_LINE_SZ/4) {
            x &= Te[i][j];
        }
    }

    return x;
}


inline word32 AES::PreFetchTd() const
{
    word32 x = 0;

    /* 4 tables of 256 entries */
    for (int i = 0; i < 4; i++) {
        /* each entry is 4 bytes */
        for (int j = 0; j < 256; j += TC_CACHE_LINE_SZ/4) {
            x &= Td[i][j];
        }
    }

    return x;
}


inline word32 AES::PreFetchCTd4() const
{
    word32 x = 0;
    int i;

    for (i = 0; i < 256; i += TC_CACHE_LINE_SZ) {
        x &= CTd4[i];
    }

    return x;
}


typedef BlockCipher<ENCRYPTION, AES, ECB> AES_ECB_Encryption;
typedef BlockCipher<DECRYPTION, AES, ECB> AES_ECB_Decryption;

typedef BlockCipher<ENCRYPTION, AES, CBC> AES_CBC_Encryption;
typedef BlockCipher<DECRYPTION, AES, CBC> AES_CBC_Decryption;


} // naemspace

#endif // TAO_CRYPT_AES_HPP
