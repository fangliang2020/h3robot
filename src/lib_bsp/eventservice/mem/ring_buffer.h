#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct ring_buffer {
    void *buffer;            //缓冲区
    uint32_t size;           //大小
    uint32_t in;             //入口位置
    uint32_t out;            //出口位置
    pthread_mutex_t *f_lock; //互斥锁
};

struct ring_buffer *ring_buffer_init(void *buffer, uint32_t size, pthread_mutex_t *f_lock);
struct ring_buffer *ring_buffer_alloc(unsigned int size);
void ring_buffer_free(struct ring_buffer *ring_buf);
uint32_t ring_buffer_len(const struct ring_buffer *ring_buf);
uint32_t ring_buffer_get(struct ring_buffer *ring_buf, void *buffer, uint32_t size);
uint32_t ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t size);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif

