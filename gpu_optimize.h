// GPU and performance optimizations for ccminer
// This module contains lock-free logging, GPU occupancy optimization, and other performance enhancements

#ifndef GPU_OPTIMIZE_H
#define GPU_OPTIMIZE_H

#include "miner.h"
#include "algos.h"
#include <cuda_runtime.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 1. LOCK-FREE RING BUFFER LOGGING (avoids mutex in hot path)
// ============================================================================

#define LOG_BUFFER_SIZE 8192  // Must be power of 2
#define LOG_ENTRY_SIZE 256

typedef struct {
	uint64_t write_pos;  // atomic counter
	uint64_t read_pos;   // soft read position (for flush)
	char buffer[LOG_BUFFER_SIZE];
} lock_free_log_t;

extern lock_free_log_t gpu_log_buffer;

// Fast lock-free append to log buffer (no mutex)
static inline void gpu_log_fast(const char *msg) {
	uint32_t len = strlen(msg);
	if (len > LOG_ENTRY_SIZE - 1) len = LOG_ENTRY_SIZE - 1;
	
	uint64_t pos = __sync_fetch_and_add(&gpu_log_buffer.write_pos, len + 1);
	uint32_t idx = (pos & (LOG_BUFFER_SIZE - 1));
	
	if (idx + len + 1 < LOG_BUFFER_SIZE) {
		memcpy(&gpu_log_buffer.buffer[idx], msg, len);
		gpu_log_buffer.buffer[idx + len] = '\n';
	}
}

// ============================================================================
// 2. NONCE ALIGNMENT FOR BETTER GPU WARP EFFICIENCY
// ============================================================================

// Align nonce ranges to warp boundaries (32 threads per warp)
// For optimal GPU utilization with strided memory access
#define NONCE_ALIGN_BITS 8  // 256-byte alignment
#define NONCE_ALIGN_MASK (~((1U << NONCE_ALIGN_BITS) - 1))

static inline uint32_t align_nonce_start(uint32_t nonce) {
	return nonce & NONCE_ALIGN_MASK;
}

static inline uint32_t align_nonce_end(uint32_t nonce) {
	return (nonce + (1U << NONCE_ALIGN_BITS)) & NONCE_ALIGN_MASK;
}

// ============================================================================
// 3. GPU OCCUPANCY OPTIMIZATION
// ============================================================================

// Helper to calculate optimal grid/block size for current GPU
typedef struct {
	int block_size;
	int grid_size;
	int occupancy;
} gpu_occupancy_t;

// Cache occupancy info per algorithm (initialized once at startup)
extern gpu_occupancy_t gpu_occupancy[100];

// Get optimal block size for kernel (must be called with actual kernel function)
#define GET_OPTIMAL_BLOCK_SIZE(kernel_func, device) \
	do { \
		int blockSize, gridSize; \
		cudaOccupancyMaxPotentialBlockSize(&gridSize, &blockSize, (void*)kernel_func, 0, 0); \
		(blockSize); \
	} while(0)

// ============================================================================
// 4. CONDITIONAL GPU SYNC (only in debug mode)
// ============================================================================

#define GPU_CHECK_ERROR(debug_mode) \
	do { if (debug_mode) { \
		cudaError_t err = cudaGetLastError(); \
		if (err != cudaSuccess) { \
			fprintf(stderr, "CUDA Error: %s\n", cudaGetErrorString(err)); \
		} \
	}} while(0)

// Non-blocking error check (doesn't synchronize GPU)
#define GPU_CHECK_ERROR_ASYNC() \
	do { \
		cudaError_t err = cudaGetLastError(); \
		if (err != cudaSuccess && err != cudaErrorCudartUnloading) { \
			/* Queue error for later async handling */ \
		} \
	} while(0)

// ============================================================================
// 5. WORK PREFETCH HINTS FOR CPU CACHE
// ============================================================================

#define PREFETCH_WORK(work_ptr) __builtin_prefetch((work_ptr), 0, 3)
#define PREFETCH_DATA(ptr) __builtin_prefetch((ptr), 0, 2)

// ============================================================================
// 6. ALGORITHM DISPATCH VIA FUNCTION POINTERS (replaces giant switch)
// ============================================================================

typedef int (*scanhash_fn)(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);

// Function pointer array indexed by algorithm ID (from algos.h)
extern scanhash_fn scanhash_functions[ALGO_COUNT];

// Initialize function pointer table (called once at startup)
void init_scanhash_functions(void);

// Dispatch algorithm - single indirect call instead of 60+ case statements
static inline int dispatch_scanhash(int algo_id, int thr_id, struct work *work, 
                                     uint32_t max_nonce, uint64_t *hashes_done) {
	if (unlikely(algo_id < 0 || algo_id >= ALGO_COUNT)) {
		return 0;
	}
	scanhash_fn fn = scanhash_functions[algo_id];
	if (unlikely(!fn)) {
		return 0;
	}
	return fn(thr_id, work, max_nonce, hashes_done);
}

#ifdef __cplusplus
}
#endif

#endif // GPU_OPTIMIZE_H
