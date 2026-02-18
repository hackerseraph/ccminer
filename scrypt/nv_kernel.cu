#include <cuda_runtime.h>

#include "salsa_kernel.h"
#include "nv_kernel.h"

NVKernel::NVKernel() : KernelInterface()
{
}

void NVKernel::set_scratchbuf_constants(int MAXWARPS, uint32_t** h_V)
{
	(void)MAXWARPS;
	(void)h_V;
}

bool NVKernel::bindtexture_1D(uint32_t *d_V, size_t size)
{
	(void)d_V;
	(void)size;
	return false;
}

bool NVKernel::bindtexture_2D(uint32_t *d_V, int width, int height, size_t pitch)
{
	(void)d_V;
	(void)width;
	(void)height;
	(void)pitch;
	return false;
}

bool NVKernel::unbindtexture_1D()
{
	return true;
}

bool NVKernel::unbindtexture_2D()
{
	return true;
}

bool NVKernel::run_kernel(dim3 grid, dim3 threads, int WARPS_PER_BLOCK, int thr_id, cudaStream_t stream,
	uint32_t* d_idata, uint32_t* d_odata, unsigned int N, unsigned int LOOKUP_GAP,
	bool interactive, bool benchmark, int texture_cache)
{
	(void)grid;
	(void)threads;
	(void)WARPS_PER_BLOCK;
	(void)thr_id;
	(void)stream;
	(void)d_idata;
	(void)d_odata;
	(void)N;
	(void)LOOKUP_GAP;
	(void)interactive;
	(void)benchmark;
	(void)texture_cache;
	return false;
}
