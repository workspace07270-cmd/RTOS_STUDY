#include <stddef.h>

/*
 * bare-metal firmware는 일반 PC 프로그램처럼 libc와 자동으로 링크되지 않습니다.
 *
 * FreeRTOS Kernel 또는 compiler가 memset/memcpy/memmove 같은 기본 메모리 함수를
 * 필요로 할 수 있으므로, 수업 예제에 필요한 최소 구현을 직접 제공합니다.
 */

void *memset(void *dest, int value, size_t count) {
    unsigned char *out = (unsigned char *) dest;

    for (size_t i = 0; i < count; i++) {
        out[i] = (unsigned char) value;
    }

    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    unsigned char *out = (unsigned char *) dest;
    const unsigned char *in = (const unsigned char *) src;

    /*
     * memcpy는 source와 destination이 겹치지 않는다고 가정합니다.
     * 단순한 byte 복사만 수행합니다.
     */
    for (size_t i = 0; i < count; i++) {
        out[i] = in[i];
    }

    return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
    unsigned char *out = (unsigned char *) dest;
    const unsigned char *in = (const unsigned char *) src;

    /*
     * memmove는 source와 destination이 겹쳐도 안전해야 합니다.
     * destination이 source 뒤쪽에 있으면 뒤에서 앞으로 복사합니다.
     */
    if (out < in) {
        for (size_t i = 0; i < count; i++) {
            out[i] = in[i];
        }
    } else {
        for (size_t i = count; i > 0; i--) {
            out[i - 1u] = in[i - 1u];
        }
    }

    return dest;
}
