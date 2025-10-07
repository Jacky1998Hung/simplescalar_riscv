#ifndef CAPTURE_BUF_H
#define CAPTURE_BUF_H

#include <stddef.h>
#include <stdio.h>
#include "ring_buffer.h"  /* for RING_BUFFER_ENTRY_BYTES */

#ifndef CAP_ENTRY_BYTES
#define CAP_ENTRY_BYTES RING_BUFFER_ENTRY_BYTES
#endif

typedef struct {
    int  valid;
    int  cycle;
    char entry[CAP_ENTRY_BYTES];
} CapEntry;

typedef struct {
    CapEntry *v;
    size_t    len;
    size_t    cap;
} CaptureBuf;

void cap_init   (CaptureBuf *cb, size_t cap);
void cap_reset  (CaptureBuf *cb);
void cap_free   (CaptureBuf *cb);
void cap_append (CaptureBuf *cb, int cycle, const char *line);
void cap_flush  (const CaptureBuf *cb, FILE *out);

#endif
