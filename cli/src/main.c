#include <stddef.h>
#include <stdio.h>

#include <jpeglib.h>
#include <sodium.h>

int main(void) {
    if (sodium_init() < 0) {
        return 1;
    }
    printf("TODO %s %d\n", sodium_version_string(), JPEG_LIB_VERSION);
    return 0;
}
