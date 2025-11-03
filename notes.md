All angles in degrees (as it is simpler whole numbers)
Top left is 0,0


| Type        | Bits | Minimum Value | Maximum Value |
|--------------|------|----------------|----------------|
| **int8_t**   | 8    | -128           | 127            |
| **uint8_t**  | 8    | 0              | 255            |
| **int16_t**  | 16   | -32,768        | 32,767         |
| **uint16_t** | 16   | 0              | 65,535         |
| **int24_t*** | 24   | -8,388,608     | 8,388,607      |
| **uint24_t***| 24   | 0              | 16,777,215     |

\*Note: `int24_t` and `uint24_t` are not standard C types, but their ranges are shown as if they were 24-bit integers.



Core Colors number
Black    = 0
White    = 255
Red      = 224
Green    = 7
Blue     = 26
Sky Blue = 30

make clean && make debug
make clean && make