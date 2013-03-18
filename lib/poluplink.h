#ifndef POLUPLINK_H
#define POLUPLINK_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t poluplink_export_wav(FILE *wavout, const uint8_t *rawbytes, size_t cnt);

#ifdef __cplusplus
}
#endif

#endif
