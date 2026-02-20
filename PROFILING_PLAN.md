# SHA256d Performance Profiling & Next Optimization Steps

## Key Finding: Naive Optimization Hurts Performance

**Test Result**: Vectorized uint2 loads → **-1% regression (1726 → 1708 MH/s)**

**Why**: Loaded dat[3] then immediately overwrote with nonce = wasted memory bandwidth. Compiler's original code was already more optimal.

**Lesson**: Hand-optimization without profiling data is counterproductive. CUDA 12.4 compiler is excellent at auto-optimization.

---

## Current Performance Ceiling

- **Best Config**: TPB 256 MB 2 NPT 1 K shared = **1726 MH/s** (Titan X Pascal)
- **Target**: 2625+ MH/s (+50%)
- **Bottleneck Status**: Unknown (memory-bandwidth suspected, not validated)

### Why 50% Gain is Hard:
1. SHA256d is **inherently memory-limited** (not compute-limited)
2. 480 GB/s theoretical BW on Titan X, at ~1750 MH/s = near saturation
3. Each hash requires ~256 bytes random-access data


---

## Data-Driven Analysis Plan

### Step 1: Profile with Nsight Compute (5 min)
```bash
# Install (if needed):
# sudo apt-get install nvidia-cuda-toolkit-12-4 nvidia-utils

# Profile best configuration:
ncu --set full -k sha256d_gpu_hash_shared --export profile.ncu-rep \
    ./ccminer --sha256d-bench 2>&1 | grep "TPB 256"

# Then open profile.ncu-rep in Nsight GUI or analyze reports/
```

**What we'll learn**:
- **Memory Throughput Utilization** % (are we hitting 480 GB/s limit?)
- **L1 Cache Hit Rate** (is latency or bandwidth the real bottleneck?)
- **Occupancy %** (could we run more warps?)
- **Warp Stall Reasons** (waiting for memory? data dependency?)
- **Instruction Cache Miss Rate** (PTX code size issue?)

### Step 2: Targeted Analysis Based on Findings

#### If Memory BW is saturated (>90%):
**Early rejection sampling is the ONLY viable path** (50-70% gain potential)
- Modify algorithm to compute partial hash after round N
- Reject non-viable nonces before full 64 rounds
- Requires loop restructuring, NOT memory access changes

#### If L1 Hit Rate is low (<60%):
**Data layout restructuring** (20-30% gain potential)
- Change from Array-of-Structs → Struct-of-Arrays
- Reorganize work so coalesced threads access same memory cells
- More complex refactoring

#### If Occupancy is <80%:
**Reduce register pressure** (5-15% gain potential)
- Lower NONCES_PER_THREAD or unroll depth
- Reduce K constant duplication

#### If Warp Stalls on L2 memory:
**Async prefetch with cp.async** (10-15% gain potential)
- CUDA 12+ feature to hide memory latency
- Compatible with current architecture (SM 6.1+)

---

## Secondary Optimization Candidates (No Profiling Needed)

### 3. Loop Strength Reduction (2-5% gain)
```cuda
// Current: shift operations inside hot loop
uint32_t s0 = ROTR32(a, 2) ^ ROTR32(a, 13) ^ ROTR32(a, 22);

// Optimized: precompute rotation into smaller operations
#pragma unroll
for (int i = 0; i < 64; ++i) {
    // Let compiler PATTERN MATCH on (a >> 2) XOR (a >> 13) XOR (a >> 22)
    // -> Becomes single SHFL + LOP3 instruction
}
```

### 4. Early Loop Termination Pattern (Algorithmic, 50-70% if viable)
```cuda
// After round 32: if hash_so_far > c_target[0], skip to next nonce
// Only ~0.01% of hashes will pass, so 99.99% can exit early
```

---

## Immediate Next Steps

**Priority 1**: Run Nsight Compute profiler
- Takes 10-15 minutes
- Gives definitive data on actual bottleneck
- Prevents more guessing

**Priority 2**: Based on profiler output, pick one optimization:
- Memory saturated? → Early-exit sampling
- L1 hits poor? → Data layout restructuring  
- Occupancy low? → Register pressure reduction

**Priority 3**: Implement + re-profile to measure gains

---

## Why We're Stuck at 1726 MH/s

1. **GPU is memory-bound**: Titan X Pascal can sustain ~1800-1850 MH/s max for SHA256d
2. **Kernel is well-optimized for parameter space**: TPB 256, MB 2, NPT 1 is likely peak for this architecture
3. **Real gains require algorithmic changes**, not just tuning knobs

To reach 2625 MH/s (+50%):
- Need either: **2 GPUs** (guaranteed 2×)
- Or: **Early-exit sampling** (~33-50% potential if partial hashes available)
- Or: **Multi-GPU + optimized batching** (complex interaction effects)

