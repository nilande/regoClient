#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "regoSerialIO.h"

/* Prototypes for internal functions */
int16_t decodeInt(char* buffer);
void encodeInt(char* buffer, int16_t number);

int main(void) {
    int16_t values[] = {0, 1, -1, 1234, -1234, 16384, -16384, 32767, -32768};
    size_t num_values = sizeof(values) / sizeof(values[0]);
    char buf[3];

    for (size_t i = 0; i < num_values; ++i) {
        encodeInt(buf, values[i]);
        int16_t decoded = decodeInt(buf);
        assert(decoded == values[i]);
    }

    puts("All encode/decode tests passed!");
    return 0;
}

