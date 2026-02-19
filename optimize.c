// Memory and work batching optimizations
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ============================================================================
// WORK BATCH QUEUE FOR REDUCED LOCK CONTENTION
// ============================================================================

#define WORK_BATCH_SIZE 16

typedef struct {
	struct work *works[WORK_BATCH_SIZE];
	int count;
	pthread_spinlock_t lock;  // Lower latency than mutex
} work_batch_t;

static work_batch_t submit_batch = {0};
static work_batch_t fetch_batch = {0};

void work_batch_init(void) {
	pthread_spin_init(&submit_batch.lock, PTHREAD_PROCESS_PRIVATE);
	pthread_spin_init(&fetch_batch.lock, PTHREAD_PROCESS_PRIVATE);
	submit_batch.count = 0;
	fetch_batch.count = 0;
}

void work_batch_add(struct work *w) {
	pthread_spin_lock(&submit_batch.lock);
	if (submit_batch.count < WORK_BATCH_SIZE) {
		submit_batch.works[submit_batch.count++] = w;
		if (submit_batch.count == WORK_BATCH_SIZE) {
			// Batch full - could trigger flush here
		}
	}
	pthread_spin_unlock(&submit_batch.lock);
}

// ============================================================================
// CUDA STREAM POOLING FOR BETTER PIPELINING
// ============================================================================

#define NUM_COMPUTE_STREAMS 4

typedef struct {
	cudaStream_t streams[NUM_COMPUTE_STREAMS];
	int next_stream;
	pthread_spinlock_t lock;
} cuda_stream_pool_t;

static cuda_stream_pool_t stream_pool = {0};

void cuda_stream_pool_init(void) {
	pthread_spin_init(&stream_pool.lock, PTHREAD_PROCESS_PRIVATE);
	
	for (int i = 0; i < NUM_COMPUTE_STREAMS; i++) {
		// Create non-blocking streams for better pipelining
		cudaStreamCreateWithFlags(&stream_pool.streams[i], 
		                         cudaStreamNonBlocking);
	}
	stream_pool.next_stream = 0;
}

cudaStream_t get_compute_stream(void) {
	pthread_spin_lock(&stream_pool.lock);
	cudaStream_t stream = stream_pool.streams[stream_pool.next_stream];
	stream_pool.next_stream = (stream_pool.next_stream + 1) % NUM_COMPUTE_STREAMS;
	pthread_spin_unlock(&stream_pool.lock);
	return stream;
}

void cuda_stream_pool_cleanup(void) {
	for (int i = 0; i < NUM_COMPUTE_STREAMS; i++) {
		cudaStreamDestroy(stream_pool.streams[i]);
	}
}

// ============================================================================
// GPU MEMORY POOLING FOR FAST ALLOCATION
// ============================================================================

typedef struct gpu_mem_block {
	void *ptr;
	size_t size;
	struct gpu_mem_block *next;
} gpu_mem_block_t;

typedef struct {
	gpu_mem_block_t *free_list;
	gpu_mem_block_t *allocated;
	pthread_spinlock_t lock;
	size_t total_allocated;
} gpu_mem_pool_t;

static gpu_mem_pool_t gpu_mem_pool = {0};

void gpu_mem_pool_init(size_t pre_allocate_size) {
	pthread_spin_init(&gpu_mem_pool.lock, PTHREAD_PROCESS_PRIVATE);
	gpu_mem_pool.free_list = NULL;
	gpu_mem_pool.allocated = NULL;
	gpu_mem_pool.total_allocated = 0;
	
	// Pre-allocate chunks
	if (pre_allocate_size > 0) {
		void *ptr;
		cudaMalloc(&ptr, pre_allocate_size);
		gpu_mem_block_t *block = (gpu_mem_block_t *)malloc(sizeof(gpu_mem_block_t));
		block->ptr = ptr;
		block->size = pre_allocate_size;
		block->next = gpu_mem_pool.free_list;
		gpu_mem_pool.free_list = block;
	}
}

void* gpu_mem_alloc(size_t size, cudaStream_t stream) {
	void *ptr = NULL;
	
	pthread_spin_lock(&gpu_mem_pool.lock);
	
	// Try to find block from free list
	gpu_mem_block_t *block = gpu_mem_pool.free_list;
	gpu_mem_block_t *prev = NULL;
	while (block) {
		if (block->size >= size) {
			ptr = block->ptr;
			if (prev) prev->next = block->next;
			else gpu_mem_pool.free_list = block->next;
			
			// Add to allocated list
			block->next = gpu_mem_pool.allocated;
			gpu_mem_pool.allocated = block;
			break;
		}
		prev = block;
		block = block->next;
	}
	
	pthread_spin_unlock(&gpu_mem_pool.lock);
	
	// If no block found, allocate new
	if (!ptr) {
		cudaMalloc(&ptr, size);
		gpu_mem_pool.total_allocated += size;
	}
	
	return ptr;
}

void gpu_mem_free(void *ptr) {
	if (!ptr) return;
	
	pthread_spin_lock(&gpu_mem_pool.lock);
	
	// Find block in allocated list and move to free list
	gpu_mem_block_t *block = gpu_mem_pool.allocated;
	gpu_mem_block_t *prev = NULL;
	while (block) {
		if (block->ptr == ptr) {
			if (prev) prev->next = block->next;
			else gpu_mem_pool.allocated = block->next;
			
			block->next = gpu_mem_pool.free_list;
			gpu_mem_pool.free_list = block;
			break;
		}
		prev = block;
		block = block->next;
	}
	
	pthread_spin_unlock(&gpu_mem_pool.lock);
}

void gpu_mem_pool_cleanup(void) {
	// Free all blocks
	gpu_mem_block_t *block;
	
	// Free allocated blocks (shouldn't happen if cleanup is done properly)
	block = gpu_mem_pool.allocated;
	while (block) {
		gpu_mem_block_t *next = block->next;
		cudaFree(block->ptr);
		free(block);
		block = next;
	}
	
	// Free free list blocks
	block = gpu_mem_pool.free_list;
	while (block) {
		gpu_mem_block_t *next = block->next;
		cudaFree(block->ptr);
		free(block);
		block = next;
	}
}

// ============================================================================
// COMPUTE QUEUE FOR WORK PREFETCHING
// ============================================================================

typedef struct {
	struct work *next_work;
	pthread_mutex_t lock;
	pthread_cond_t ready;
	bool has_work;
} prefetch_queue_t;

static prefetch_queue_t prefetch = {0};

void prefetch_queue_init(void) {
	pthread_mutex_init(&prefetch.lock, NULL);
	pthread_cond_init(&prefetch.ready, NULL);
	prefetch.has_work = false;
	prefetch.next_work = NULL;
}

void prefetch_queue_push(struct work *w) {
	pthread_mutex_lock(&prefetch.lock);
	prefetch.next_work = w;
	prefetch.has_work = true;
	pthread_cond_signal(&prefetch.ready);
	pthread_mutex_unlock(&prefetch.lock);
}

struct work* prefetch_queue_pop_nowait(void) {
	struct work *w = NULL;
	if (pthread_mutex_trylock(&prefetch.lock) == 0) {
		if (prefetch.has_work) {
			w = prefetch.next_work;
			prefetch.has_work = false;
		}
		pthread_mutex_unlock(&prefetch.lock);
	}
	return w;
}
