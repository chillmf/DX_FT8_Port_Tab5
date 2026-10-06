
#ifndef ENCODE_H_
#define ENCODE_H_

#include <stdint.h>

// Encode an 87-bit message and return a 174-bit codeword.
// The generator matrix has dimensions (87,87).
// The code is a (174,87) regular ldpc code with column weight 3.
// The code was generated using the PEG algorithm.
// After creating the codeword, the columns are re-ordered according to
// "colorder" to make the codeword compatible with the parity-check matrix
// Arguments:
//   * message - array of 87 bits stored as 11 bytes (MSB first)
//   * codeword - array of 174 bits stored as 22 bytes (MSB first)
void encode174(const uint8_t *message, uint8_t *codeword);

// Compute 14-bit CRC for a sequence of given number of bits
// [IN] message  - byte sequence (MSB first)
// [IN] num_bits - number of bits in the sequence
uint16_t crc(uint8_t *message, int num_bits);

#endif // ENCODE_H_
