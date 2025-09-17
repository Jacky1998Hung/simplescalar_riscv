#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>  // size_t
#include <stdio.h>   // FILE*

#ifdef __cplusplus
extern "C" {
#endif

/* You can override this at compile time, e.g.:
 *   gcc -DRING_BUFFER_SIZE=256 ...
 */
#ifndef RING_BUFFER_SIZE
#define RING_BUFFER_SIZE 128
#endif

#ifndef RING_BUFFER_ENTRY_BYTES
#define RING_BUFFER_ENTRY_BYTES 128
#endif

typedef struct {
    int  valid;                                   /* 1 = valid */
    int  cycle;                                   /* your cycle or timestamp */
    char entry[RING_BUFFER_ENTRY_BYTES];          /* formatted log line */
} TraceEntry;

typedef struct {
    TraceEntry buffer[RING_BUFFER_SIZE];
    size_t head;         /* write position (next slot to write) */
    size_t tail;         /* oldest entry */
    int    full;         /* distinguish full vs empty when head == tail */
} RingBuffer;

/* Initialize / reset */
void   rb_init(RingBuffer *rb);
void   rb_clear(RingBuffer *rb);

/* Push APIs */
void   rb_push_raw(RingBuffer *rb, int cycle, const char *text);
void   rb_pushf   (RingBuffer *rb, int cycle, const char *fmt, ...);

/* Basic queries */
int    rb_is_empty(const RingBuffer *rb);
int    rb_is_full (const RingBuffer *rb);
size_t rb_capacity(const RingBuffer *rb);       /* = RING_BUFFER_SIZE */
size_t rb_count   (const RingBuffer *rb);       /* number of valid entries */

/* Dump helpers (write to FILE*) */
void   rb_flush(const RingBuffer *rb, FILE *out);               /* oldest → newest */
void   rb_dump_before(const RingBuffer *rb, size_t N, FILE *out); /* N entries before head */
void   rb_dump_after (const RingBuffer *rb, size_t N, FILE *out); /* N entries starting at head */

/* Visitor iteration: oldest → newest */
typedef void (*rb_visit_fn)(const TraceEntry *e, void *ctx);
void   rb_visit(const RingBuffer *rb, rb_visit_fn fn, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */

