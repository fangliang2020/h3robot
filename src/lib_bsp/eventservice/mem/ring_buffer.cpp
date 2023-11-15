#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "ring_buffer.h"

pthread_mutex_t ring_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

//判断x是否是2的次方
#define is_power_of_2(x) ((x) != 0 && (((x) & ((x)-1)) == 0))
//取a和b中最小值
#define min(a, b) (((a) < (b)) ? (a) : (b))

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int fls(int x) {
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

static inline int fls64(uint64_t x) {
    uint32_t h = x >> 32;
    if (h)
        return fls(h) + 32;
    return fls(x);
}

static inline unsigned fls_long(unsigned long l) {
    if (sizeof(l) == 4)
        return fls(l);
    return fls64(l);
}

static inline unsigned long roundup_pow_of_two(unsigned long x) {
    return 1UL << fls_long(x - 1);
}

//初始化缓冲区
struct ring_buffer *ring_buffer_init(void *buffer, uint32_t size,
                                     pthread_mutex_t *f_lock) {
    assert(buffer);
    struct ring_buffer *ring_buf = NULL;

    if (!is_power_of_2(size)) {
        fprintf(stderr, "size must be power of 2.\n");
        return NULL;
    }

    ring_buf = (struct ring_buffer *) malloc(sizeof(struct ring_buffer));
    if (!ring_buf) {
        fprintf(stderr, "Failed to malloc memory, %s", strerror(errno));
        return NULL;
    }

    memset(ring_buf, 0, sizeof(struct ring_buffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in = 0;
    ring_buf->out = 0;
    ring_buf->f_lock = f_lock;

    return ring_buf;
}

struct ring_buffer *ring_buffer_alloc(unsigned int size) {
    unsigned char *buffer;
    struct ring_buffer *ret;
    if (size & (size - 1)) {
        fprintf(stderr, "size > 0x80000000n");
        size = roundup_pow_of_two(size);
    }

    buffer = (unsigned char *)malloc(size);
    if (!buffer)
        return NULL;

    ret = ring_buffer_init(buffer, size, &ring_buffer_mutex);

    if ((unsigned long)ret <= 0) {
        free(buffer);
    }

    return ret;
}

//释放缓冲区
void ring_buffer_free(struct ring_buffer *ring_buf) {
    if (ring_buf) {
        if (ring_buf->buffer) {
            free(ring_buf->buffer);
            ring_buf->buffer = NULL;
        }
        free(ring_buf);
        ring_buf = NULL;
    }
}

//缓冲区的长度
uint32_t __ring_buffer_len(const struct ring_buffer *ring_buf) {
    return (ring_buf->in - ring_buf->out);
}

//从缓冲区中取数据
uint32_t __ring_buffer_get(struct ring_buffer *ring_buf, void *buffer, uint32_t size) {
    assert(ring_buf || buffer);
    uint32_t len = 0;
    size = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, (uint32_t*)ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1)), len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy((uint32_t*)buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    return size;
}

//向缓冲区中存放数据
uint32_t __ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t size) {
    assert(ring_buf || buffer);
    uint32_t len = 0;
    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    len = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
    memcpy((uint32_t*)ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer, len);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, (uint32_t*)buffer + len, size - len);
    ring_buf->in += size;
    return size;
}

uint32_t ring_buffer_len(const struct ring_buffer *ring_buf) {
    uint32_t len = 0;
    pthread_mutex_lock(ring_buf->f_lock);
    len = __ring_buffer_len(ring_buf);
    pthread_mutex_unlock(ring_buf->f_lock);
    return len;
}

uint32_t ring_buffer_get(struct ring_buffer *ring_buf, void *buffer, uint32_t size) {
    uint32_t ret;
    pthread_mutex_lock(ring_buf->f_lock);
    ret = __ring_buffer_get(ring_buf, buffer, size);
    // buffer中没有数据
    if (ring_buf->in == ring_buf->out)
        ring_buf->in = ring_buf->out = 0;
    pthread_mutex_unlock(ring_buf->f_lock);
    return ret;
}

uint32_t ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t size) {
    uint32_t ret;
    pthread_mutex_lock(ring_buf->f_lock);
    ret = __ring_buffer_put(ring_buf, buffer, size);
    pthread_mutex_unlock(ring_buf->f_lock);
    return ret;
}
