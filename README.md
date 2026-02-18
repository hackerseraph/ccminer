# ccminer

Based on Christian Buchner's &amp; Christian H.'s CUDA project, no more active on github since 2014.

Check the [README.txt](README.txt) for the additions

BTC donation address: 1AJdfCpLWPNoAMDfHF1wD5y8VgKSSTHxPo (tpruvot)

A part of the recent algos were originally written by [djm34](https://github.com/djm34) and [alexis78](https://github.com/alexis78)

Historically, this variant was tested on Linux (Ubuntu 14.04/16.04, Fedora 22 to 25).
Historically, Windows builds targeted Windows 7 to 10 with Visual Studio 2013.

Note that the x86 releases are generally faster than x64 ones on Windows, but that tend to change with the recent drivers.

The recommended CUDA Toolkit version was the [6.5.19](http://developer.download.nvidia.com/compute/cuda/6_5/rel/installers/cuda_6.5.19_windows_general_64.exe), but some light algos could be faster with the version 7.5 and 8.0 (like lbry, decred and skein).

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
