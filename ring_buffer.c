#include "ring_buffer.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static inline size_t rb_next(size_t i) {
    return (i + 1) % RING_BUFFER_SIZE;
}

void rb_init(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->full = 0;
    for (size_t i = 0; i < RING_BUFFER_SIZE; ++i) {
        rb->buffer[i].valid = 0;
        rb->buffer[i].cycle = 0;
        rb->buffer[i].entry[0] = '\0';
    }
}

void rb_clear(RingBuffer *rb) {
    rb_init(rb);
}

int rb_is_empty(const RingBuffer *rb) {
    return (!rb->full && rb->head == rb->tail);
}

int rb_is_full(const RingBuffer *rb) {
    return rb->full;
}

size_t rb_capacity(const RingBuffer *rb) {
    (void)rb;
    return RING_BUFFER_SIZE;
}

size_t rb_count(const RingBuffer *rb) {
    if (rb->full) return RING_BUFFER_SIZE;
    if (rb->head >= rb->tail) return rb->head - rb->tail;
    return RING_BUFFER_SIZE - (rb->tail - rb->head);
}

static void rb_advance_head(RingBuffer *rb) {
    rb->head = rb_next(rb->head);
    if (rb->full) {
        /* overwriting oldest -> move tail forward */
        rb->tail = rb_next(rb->tail);
    }
    rb->full = (rb->head == rb->tail);
}

void rb_push_raw(RingBuffer *rb, int cycle, const char *text) {
    TraceEntry *e = &rb->buffer[rb->head];
    e->cycle = cycle;
    e->valid = 1;
    if (text) {
        /* Ensure null-terminated copy */
        strncpy(e->entry, text, RING_BUFFER_ENTRY_BYTES - 1);
        e->entry[RING_BUFFER_ENTRY_BYTES - 1] = '\0';
    } else {
        e->entry[0] = '\0';
    }
    rb_advance_head(rb);
}

void rb_pushf(RingBuffer *rb, int cycle, const char *fmt, ...) {
    TraceEntry *e = &rb->buffer[rb->head];
    e->cycle = cycle;
    e->valid = 1;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(e->entry, RING_BUFFER_ENTRY_BYTES, fmt, args);
        va_end(args);
    } else {
        e->entry[0] = '\0';
    }
    rb_advance_head(rb);
}

/* Visit entries from oldest (tail) to newest (head-1) */
void rb_visit(const RingBuffer *rb, rb_visit_fn fn, void *ctx) {
    if (!fn || rb_is_empty(rb)) return;

    size_t i = rb->tail;
    size_t remaining = rb_count(rb);

    while (remaining--) {
        const TraceEntry *e = &rb->buffer[i];
        if (e->valid) fn(e, ctx);
        i = rb_next(i);
    }
}

void rb_flush(const RingBuffer *rb, FILE *out) {
    if (!out || rb_is_empty(rb)) return;

    size_t i = rb->tail;
    size_t remaining = rb_count(rb);

    while (remaining--) {
        const TraceEntry *e = &rb->buffer[i];
        if (e->valid) {
            /* output the string we store */
            fprintf(out, "%s\n", e->entry);
        }
        i = rb_next(i);
    }
}

/* Dump N entries immediately prior to current head (most-recent-first index, but we print in chronological order) */
void rb_dump_before(const RingBuffer *rb, size_t N, FILE *out) {
    if (!out || N == 0 || rb_is_empty(rb)) return;

    size_t total = rb_count(rb);
    if (N > total) N = total;

    /* Start index: head - N (wrapped) */
    size_t start = (rb->head + RING_BUFFER_SIZE - N) % RING_BUFFER_SIZE;

    for (size_t k = 0; k < N; ++k) {
        size_t idx = (start + k) % RING_BUFFER_SIZE;
        const TraceEntry *e = &rb->buffer[idx];
        if (e->valid) fprintf(out, "%s", e->entry);
    }
}

/* Dump N entries starting at head (look-ahead window).
 * Note: these are entries that may have been pre-filled by the caller for “future” cycles.
 * If you strictly push at head only, this will likely print nothing unless you’ve staged entries.
 */
void rb_dump_after(const RingBuffer *rb, size_t N, FILE *out) {
    if (!out || N == 0) return;

    for (size_t k = 0; k < N; ++k) {
        size_t idx = (rb->head + k) % RING_BUFFER_SIZE;
        const TraceEntry *e = &rb->buffer[idx];
        if (!e->valid) break;
        fprintf(out, "%s\n", e->entry);
    }
}

