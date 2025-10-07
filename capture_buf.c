#include "capture_buf.h"
#include <stdlib.h>
#include <string.h>

static void cap_grow(CaptureBuf *cb, size_t need) {
    if (cb->len + need <= cb->cap) return;
    size_t ncap = cb->cap ? cb->cap : 256;
    while (ncap < cb->len + need) ncap *= 2;
    cb->v = (CapEntry*)realloc(cb->v, ncap * sizeof(CapEntry));
    cb->cap = ncap;
}

void cap_init(CaptureBuf *cb, size_t cap) {
    cb->v = (CapEntry*)malloc(cap * sizeof(CapEntry));
    cb->len = 0;
    cb->cap = cap;
}

void cap_reset(CaptureBuf *cb) { cb->len = 0; }

void cap_free(CaptureBuf *cb) {
    free(cb->v);
    cb->v = NULL;
    cb->len = 0;
    cb->cap = 0;
}

void cap_append(CaptureBuf *cb, int cycle, const char *line) {
    cap_grow(cb, 1);
    CapEntry *e = &cb->v[cb->len++];
    e->valid = 1;
    e->cycle = cycle;
    if (line) {
        strncpy(e->entry, line, CAP_ENTRY_BYTES - 1);
        e->entry[CAP_ENTRY_BYTES - 1] = '\0';
    } else {
        e->entry[0] = '\0';
    }
}

void cap_flush(const CaptureBuf *cb, FILE *out) {
    if (!out) return;
    for (size_t i = 0; i < cb->len; ++i) {
        if (!cb->v[i].valid) continue;
        fputs(cb->v[i].entry, out);
        size_t L = strlen(cb->v[i].entry);
        if (L && cb->v[i].entry[L-1] != '\n') fputc('\n', out);
    }
}

