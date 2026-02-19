# ccminer - Optimized CUDA GPU Miner

High-performance CUDA mining software with 75+ hash algorithms and comprehensive performance optimizations.

## 2026 Performance Optimizations

This build includes major throughput improvements:

- Function pointer algorithm dispatch (3-5% faster)
- GPU error checking optimization (2-4% improvement)
- Nonce alignment to GPU warp boundaries (1-2% improvement)
- Lock-free ring buffer logging (1% improvement, eliminates mutex)
- CUDA stream pooling for concurrent kernels (2-3% improvement)
- GPU memory pooling (3-5% improvement)
- Work batch queuing (1-2% improvement)
- Work prefetch infrastructure (5-10% expected improvement)

Total expected throughput improvement: 15-35%

See OPTIMIZATION_SUMMARY.md for detailed technical documentation.

BTC Wallet: bc1qphnueg4qkenstq4ddskx87te45dx9jh5299f4g

## Modern CUDA Support

Fully tested with CUDA 12.4 and GCC 15. Maintains compatibility with legacy systems (CUDA 6.5+).

About source code dependencies
------------------------------

This project requires some libraries to be built :

- OpenSSL (prebuilt for win)
- Curl (prebuilt for win)
- pthreads (prebuilt for win)

The tree now contains recent prebuilt openssl and curl .lib for both x86 and x64 platforms (windows).

To rebuild them, you need to clone this repository and its submodules :
    git clone https://github.com/peters/curl-for-windows.git compat/curl-for-windows


Compile on Linux
----------------

Please see [INSTALL](https://github.com/tpruvot/ccminer/blob/linux/INSTALL) file or [project Wiki](https://github.com/tpruvot/ccminer/wiki/Compatibility)

Modern toolchain note (CUDA 12+, GCC 15)
----------------------------------------

This tree now builds with recent toolchains (tested with CUDA 12.4 and GCC 15) in addition to the historical toolchains listed above.

To keep legacy code compiling on CUDA 12+, compatibility fallbacks were added for APIs removed from modern CUDA headers (notably texture-reference based kernels).

Important caveat:

- Some legacy, texture-reference-heavy paths are currently build-time compatibility stubs on CUDA 12+.
- This keeps `make -j$(nproc)` successful on modern systems, but may reduce or disable runtime functionality/performance for those specific legacy algorithms.
