/*
 *  matrixCode.c
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2023 Philip Lamb
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "matrixCode.h"
#include <stdbool.h>

static inline int unpack_number(uint64_t number, int length, uint8_t *bits)
{
    for (int i = 0; i < length; i++) {
        bits[i] = number & 1;
        number = number >> 1;
    }
    return length;
}

int encode_bch(int length, int k, const int *g, uint64_t number, uint8_t **out_bits_p)
{
    *out_bits_p = (uint8_t *)calloc(length, 1);

    // Unpack the input into data.
    uint8_t * const data = *out_bits_p + length - k;
    unpack_number(number, k, data);

    // Compute redundancy bb[], the coefficients of b(x). The redundancy
    // polynomial b(x) is the remainder after dividing x^(length-k)*data(x)
    // by the generator polynomial g(x).
    uint8_t * const bb = *out_bits_p;
    for (int i = k - 1; i >= 0; i--) {
        int feedback = data[i] ^ bb[length - k - 1];
        if (feedback != 0) {
            for (int j = length - k - 1; j > 0; j--) {
                if (g[j] != 0) bb[j] = bb[j - 1] ^ feedback;
                else bb[j] = bb[j - 1];
            }
            bb[0] = g[0] && feedback;
        } else {
            for (int j = length - k - 1; j > 0; j--) bb[j] = bb[j - 1];
            bb[0] = 0;
        }
    }
    return (length);
}

int encodeMatrixCode(const AR_MATRIX_CODE_TYPE matrixCodeType, uint64_t in, uint8_t **out_bits_p)
{
    if (!out_bits_p) return 0;

    const signed char hamming63EncoderTable[8] = {0, 7, 25, 30, 42, 45, 51, 52};
    const signed char parity65EncoderTable[32] = {0, 33, 34, 3, 36, 5, 6, 39, 40, 9, 10, 43, 12, 45, 46, 15, 48, 17, 18, 51, 20, 53, 54, 23, 24, 57, 58, 27, 60, 29, 30, 63};
    const int bch_13_9_3_Galois[5] = {1, 1, 0, 0, 1};
    const int bch_13_5_5_Galois[9] = {1, 0, 0, 0, 1, 0, 1, 1, 1};
    const int bch_22_12_5_Galois[11] = {1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1};
    const int bch_22_7_7_Galois[16] = {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1};

    if (matrixCodeType == AR_MATRIX_CODE_3x3_HAMMING63) {
        if (in >= 8u) return 0;
        *out_bits_p = (uint8_t *)malloc(6);
        return (unpack_number(hamming63EncoderTable[in], 6, *out_bits_p));
    } else if (matrixCodeType == AR_MATRIX_CODE_3x3_PARITY65) {
        *out_bits_p = (uint8_t *)malloc(6);
        if (in >= 32u) return 0;
        return (unpack_number(parity65EncoderTable[in], 6, *out_bits_p));
    } else if (matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_9_3) {
        if (in >= 512u) return 0;
        return (encode_bch(13, 9, bch_13_9_3_Galois, in, out_bits_p));
    } else if (matrixCodeType == AR_MATRIX_CODE_4x4_BCH_13_5_5) {
        if (in >= 32u) return 0;
        return (encode_bch(13, 5, bch_13_5_5_Galois, in, out_bits_p));
    } else if (matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_12_5) {
        if (in >= 4096u) return 0;
        return (encode_bch(22, 12, bch_22_12_5_Galois, in, out_bits_p));
    } else if (matrixCodeType == AR_MATRIX_CODE_5x5_BCH_22_7_7) {
        if (in >= 128u) return 0;
        return (encode_bch(22, 7, bch_22_7_7_Galois, in, out_bits_p));
    } else if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID) {
        return 0; // Encoding unsupported.
    } else {
        // Raw code.
        int barcode_dimensions = matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK;
        int length = barcode_dimensions*barcode_dimensions - 3;
        if (length > 63) return 0;
        uint64_t codeCount = 1u << length;
        if (in >= codeCount) return 0;
        *out_bits_p = (uint8_t *)malloc(length);
        return (unpack_number(in, length, *out_bits_p));
    }
}
