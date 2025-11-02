#ifndef TRIG_H
#define TRIG_H

#include <stdint.h>

extern const int16_t sin_table[360];
extern const int16_t cot_table[360];
int24_t fast_sin_mul(int16_t value, uint16_t angle);
int24_t fast_cos_mul(int16_t value, uint16_t angle);
int24_t fast_tan_mul(int16_t value, uint16_t angle);
int24_t fast_cot_mul(int16_t value, uint16_t angle);

#endif // TRIG_H
