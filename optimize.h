// Batch and memory optimization header
#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include <cuda_runtime.h>
#include "miner.h"

#ifdef __cplusplus
extern "C" {
#endif

// Work batch functions
void work_batch_init(void);
void work_batch_add(struct work *w);

// CUDA stream pooling
void cuda_stream_pool_init(void);
cudaStream_t get_compute_stream(void);
void cuda_stream_pool_cleanup(void);

// GPU memory pooling
void gpu_mem_pool_init(size_t pre_allocate_size);
void* gpu_mem_alloc(size_t size, cudaStream_t stream);
void gpu_mem_free(void *ptr);
void gpu_mem_pool_cleanup(void);

// Prefetch queue
void prefetch_queue_init(void);
void prefetch_queue_push(struct work *w);
struct work* prefetch_queue_pop_nowait(void);

#ifdef __cplusplus
}
#endif

#endif
