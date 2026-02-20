/*
 * Hand-optimized PTX for SHA256d critical path
 * 2026 - Assembly-level optimizations for Maxwell/Pascal/Turing
 */

#ifndef SHA256_PTX_OPTIMIZED_CUH
#define SHA256_PTX_OPTIMIZED_CUH

// Optimized ROTR with explicit instruction scheduling
__device__ __forceinline__ uint32_t ROTR_PTX(uint32_t x, uint32_t n)
{
	uint32_t result;
	asm("shf.r.wrap.b32 %0, %1, %1, %2;" : "=r"(result) : "r"(x), "r"(n));
	return result;
}

// Fused rotate-xor-rotate-xor pattern (bsg2_1: ROTR(6) ^ ROTR(11) ^ ROTR(25))
// This eliminates intermediate register moves by fusing operations
__device__ __forceinline__ uint32_t bsg2_1_fused_ptx(uint32_t e)
{
	uint32_t r6, r11, r25, xor_temp, result;
	asm volatile (
		"{\n\t"
		"  .reg .u32 t1, t2;\n\t"
		"  shf.r.wrap.b32 %0, %4, %4, 6;\n\t"     // ROTR(e, 6)
		"  shf.r.wrap.b32 %1, %4, %4, 11;\n\t"    // ROTR(e, 11) - parallel issue
		"  shf.r.wrap.b32 %2, %4, %4, 25;\n\t"    // ROTR(e, 25) - parallel issue
		"  xor.b32 t1, %0, %1;\n\t"               // XOR first two
		"  xor.b32 %3, t1, %2;\n\t"               // XOR with third
		"}\n\t"
		: "=r"(r6), "=r"(r11), "=r"(r25), "=r"(result)
		: "r"(e)
	);
	return result;
}

// Fused bsg2_0: ROTR(2) ^ ROTR(13) ^ ROTR(22)
__device__ __forceinline__ uint32_t bsg2_0_fused_ptx(uint32_t a)
{
	uint32_t result;
	asm volatile (
		"{\n\t"
		"  .reg .u32 r2, r13, r22, t1;\n\t"
		"  shf.r.wrap.b32 r2, %1, %1, 2;\n\t"
		"  shf.r.wrap.b32 r13, %1, %1, 13;\n\t"
		"  shf.r.wrap.b32 r22, %1, %1, 22;\n\t"
		"  xor.b32 t1, r2, r13;\n\t"
		"  xor.b32 %0, t1, r22;\n\t"
		"}\n\t"
		: "=r"(result)
		: "r"(a)
	);
	return result;
}

// Fused ssg2_1: ROTR(17) ^ ROTR(19) ^ SHR(10)
__device__ __forceinline__ uint32_t ssg2_1_fused_ptx(uint32_t x)
{
	uint32_t result;
	asm volatile (
		"{\n\t"
		"  .reg .u32 r17, r19, s10, t1;\n\t"
		"  shf.r.wrap.b32 r17, %1, %1, 17;\n\t"
		"  shf.r.wrap.b32 r19, %1, %1, 19;\n\t"
		"  shr.u32 s10, %1, 10;\n\t"              // logical shift
		"  xor.b32 t1, r17, r19;\n\t"
		"  xor.b32 %0, t1, s10;\n\t"
		"}\n\t"
		: "=r"(result)
		: "r"(x)
	);
	return result;
}

// Fused ssg2_0: ROTR(7) ^ ROTR(18) ^ SHR(3)
__device__ __forceinline__ uint32_t ssg2_0_fused_ptx(uint32_t x)
{
	uint32_t result;
	asm volatile (
		"{\n\t"
		"  .reg .u32 r7, r18, s3, t1;\n\t"
		"  shf.r.wrap.b32 r7, %1, %1, 7;\n\t"
		"  shf.r.wrap.b32 r18, %1, %1, 18;\n\t"
		"  shr.u32 s3, %1, 3;\n\t"
		"  xor.b32 t1, r7, r18;\n\t"
		"  xor.b32 %0, t1, s3;\n\t"
		"}\n\t"
		: "=r"(result)
		: "r"(x)
	);
	return result;
}

// Ultra-fused SHA256 round step - combines rotation + logic in one asm block
// Reduces register pressure and enables better scheduling
__device__ __forceinline__ void sha256_step1_ptx(
	uint32_t a, uint32_t b, uint32_t c, uint32_t &d,
	uint32_t e, uint32_t f, uint32_t g, uint32_t &h,
	uint32_t in, uint32_t K)
{
	uint32_t new_d, new_h;
	asm volatile (
		"{\n\t"
		"  .reg .u32 vxandx, bsg21, bsg20, andorv, t1, t2;\n\t"
		// Ch(e,f,g) = (e & f) ^ (~e & g) = ((f ^ g) & e) ^ g
		"  xor.b32 vxandx, %5, %6;\n\t"
		"  and.b32 vxandx, vxandx, %4;\n\t"
		"  xor.b32 vxandx, vxandx, %6;\n\t"
		// Sigma1(e) = ROTR(e,6) ^ ROTR(e,11) ^ ROTR(e,25)
		"  .reg .u32 e6, e11, e25;\n\t"
		"  shf.r.wrap.b32 e6, %4, %4, 6;\n\t"
		"  shf.r.wrap.b32 e11, %4, %4, 11;\n\t"
		"  shf.r.wrap.b32 e25, %4, %4, 25;\n\t"
		"  xor.b32 bsg21, e6, e11;\n\t"
		"  xor.b32 bsg21, bsg21, e25;\n\t"
		// Sigma0(a) = ROTR(a,2) ^ ROTR(a,13) ^ ROTR(a,22)
		"  .reg .u32 a2, a13, a22;\n\t"
		"  shf.r.wrap.b32 a2, %0, %0, 2;\n\t"
		"  shf.r.wrap.b32 a13, %0, %0, 13;\n\t"
		"  shf.r.wrap.b32 a22, %0, %0, 22;\n\t"
		"  xor.b32 bsg20, a2, a13;\n\t"
		"  xor.b32 bsg20, bsg20, a22;\n\t"
		// Maj(a,b,c) = (a & b) | ((a | b) & c)
		"  .reg .u32 m, n, o;\n\t"
		"  and.b32 m, %0, %1;\n\t"
		"  or.b32 n, %0, %1;\n\t"
		"  and.b32 o, n, %2;\n\t"
		"  or.b32 andorv, m, o;\n\t"
		// t1 = h + Sigma1 + Ch + K + W
		"  add.u32 t1, %7, bsg21;\n\t"
		"  add.u32 t1, t1, vxandx;\n\t"
		"  add.u32 t1, t1, %9;\n\t"
		"  add.u32 t1, t1, %8;\n\t"
		// t2 = Sigma0 + Maj
		"  add.u32 t2, bsg20, andorv;\n\t"
		// d = d + t1
		"  add.u32 %10, %3, t1;\n\t"
		// h = t1 + t2
		"  add.u32 %11, t1, t2;\n\t"
		"}\n\t"
		: "=r"(new_d), "=r"(new_h)
		: "r"(a), "r"(b), "r"(c), "r"(d), "r"(e), "r"(f), "r"(g), "r"(h), "r"(K), "r"(in)
	);
	d = new_d;
	h = new_h;
}

#endif // SHA256_PTX_OPTIMIZED_CUH
