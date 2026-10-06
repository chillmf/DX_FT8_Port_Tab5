
#pragma once

void bp_decode(float codeword[], int max_iters, uint8_t plain[], int *ok);

// Packs a string of bits each represented as a zero/non-zero byte in plain[],
// as a string of packed bits starting from the MSB of the first byte of packed[]
void pack_bits(const uint8_t plain[], int num_bits, uint8_t packed[]);
