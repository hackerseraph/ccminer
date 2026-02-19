// GPU optimization implementations
#include "gpu_optimize.h"
#include "miner.h"
#include "algos.h"
#include <cuda_runtime.h>
#include <stdio.h>

lock_free_log_t gpu_log_buffer = {0};
gpu_occupancy_t gpu_occupancy[100] = {0};

// Function pointer array indexed by algorithm ID (all declarations are in miner.h)
scanhash_fn scanhash_functions[ALGO_COUNT] = {0};

void init_scanhash_functions(void) {
	memset(scanhash_functions, 0, sizeof(scanhash_functions));
	
	// Initialize array indexed by enum values from algos.h
	scanhash_functions[ALGO_BLAKECOIN] = (scanhash_fn)scanhash_blake256;       // Note: wrapper needed for rounds param
	scanhash_functions[ALGO_BLAKE] = (scanhash_fn)scanhash_blake256;           // Note: wrapper needed for rounds param
	scanhash_functions[ALGO_BLAKE2B] = scanhash_blake2b;
	scanhash_functions[ALGO_BLAKE2S] = scanhash_blake2s;
	scanhash_functions[ALGO_ALLIUM] = scanhash_allium;
	scanhash_functions[ALGO_BMW] = scanhash_bmw;
	scanhash_functions[ALGO_BASTION] = scanhash_bastion;
	scanhash_functions[ALGO_C11] = scanhash_c11;
	scanhash_functions[ALGO_CRYPTOLIGHT] = (scanhash_fn)scanhash_cryptolight;  // Note: wrapper needed for variant param
	scanhash_functions[ALGO_CRYPTONIGHT] = (scanhash_fn)scanhash_cryptonight;  // Note: wrapper needed for variant param
	scanhash_functions[ALGO_DEEP] = scanhash_deep;
	scanhash_functions[ALGO_DECRED] = scanhash_decred;
	scanhash_functions[ALGO_DMD_GR] = scanhash_groestlcoin;
	scanhash_functions[ALGO_EQUIHASH] = scanhash_equihash;
	scanhash_functions[ALGO_EXOSIS] = scanhash_exosis;
	scanhash_functions[ALGO_FRESH] = scanhash_fresh;
	scanhash_functions[ALGO_FUGUE256] = scanhash_fugue256;
	scanhash_functions[ALGO_GROESTL] = scanhash_groestlcoin;
	scanhash_functions[ALGO_HEAVY] = (scanhash_fn)scanhash_heavy;              // Note: wrapper needed for vote/header_size params
	scanhash_functions[ALGO_HMQ1725] = scanhash_hmq17;
	scanhash_functions[ALGO_HSR] = scanhash_hsr;
	scanhash_functions[ALGO_KECCAK] = scanhash_keccak256;
	scanhash_functions[ALGO_KECCAKC] = scanhash_keccak256;
	scanhash_functions[ALGO_JACKPOT] = scanhash_jackpot;
	scanhash_functions[ALGO_JHA] = scanhash_jha;
	scanhash_functions[ALGO_LBRY] = scanhash_lbry;
	scanhash_functions[ALGO_LUFFA] = scanhash_luffa;
	scanhash_functions[ALGO_LYRA2] = scanhash_lyra2;
	scanhash_functions[ALGO_LYRA2v2] = scanhash_lyra2v2;
	scanhash_functions[ALGO_LYRA2v3] = scanhash_lyra2v3;
	scanhash_functions[ALGO_LYRA2Z] = scanhash_lyra2Z;
	scanhash_functions[ALGO_MJOLLNIR] = (scanhash_fn)scanhash_heavy;           // Note: wrapper needed
	scanhash_functions[ALGO_MYR_GR] = scanhash_myriad;
	scanhash_functions[ALGO_NEOSCRYPT] = scanhash_neoscrypt;
	scanhash_functions[ALGO_NIST5] = scanhash_nist5;
	scanhash_functions[ALGO_PENTABLAKE] = scanhash_pentablake;
	scanhash_functions[ALGO_PHI] = scanhash_phi;
	scanhash_functions[ALGO_PHI2] = scanhash_phi2;
	scanhash_functions[ALGO_POLYTIMOS] = scanhash_polytimos;
	scanhash_functions[ALGO_QUARK] = scanhash_quark;
	scanhash_functions[ALGO_QUBIT] = scanhash_qubit;
	scanhash_functions[ALGO_SCRYPT] = (scanhash_fn)scanhash_scrypt;            // Note: wrapper needed
	scanhash_functions[ALGO_SCRYPT_JANE] = (scanhash_fn)scanhash_scrypt_jane;  // Note: wrapper needed
	scanhash_functions[ALGO_SHA256D] = scanhash_sha256d;
	scanhash_functions[ALGO_SHA256T] = scanhash_sha256t;
	scanhash_functions[ALGO_SHA256Q] = scanhash_sha256q;
	scanhash_functions[ALGO_SIA] = scanhash_sia;
	scanhash_functions[ALGO_SIB] = scanhash_sib;
	scanhash_functions[ALGO_SKEIN] = scanhash_skeincoin;
	scanhash_functions[ALGO_SKEIN2] = scanhash_skein2;
	scanhash_functions[ALGO_SKUNK] = scanhash_skunk;
	scanhash_functions[ALGO_SONOA] = scanhash_sonoa;
	scanhash_functions[ALGO_S3] = scanhash_s3;
	scanhash_functions[ALGO_TIMETRAVEL] = scanhash_timetravel;
	scanhash_functions[ALGO_TRIBUS] = scanhash_tribus;
	scanhash_functions[ALGO_BITCORE] = scanhash_bitcore;
	scanhash_functions[ALGO_X11EVO] = scanhash_x11evo;
	scanhash_functions[ALGO_X11] = scanhash_x11;
	scanhash_functions[ALGO_X12] = scanhash_x12;
	scanhash_functions[ALGO_X13] = scanhash_x13;
	scanhash_functions[ALGO_X14] = scanhash_x14;
	scanhash_functions[ALGO_X15] = scanhash_x15;
	scanhash_functions[ALGO_X16R] = scanhash_x16r;
	scanhash_functions[ALGO_X16S] = scanhash_x16s;
	scanhash_functions[ALGO_X17] = scanhash_x17;
	scanhash_functions[ALGO_VANILLA] = (scanhash_fn)scanhash_vanilla;          // Note: wrapper needed for rounds param
	scanhash_functions[ALGO_VELTOR] = scanhash_veltor;
	scanhash_functions[ALGO_WHIRLCOIN] = scanhash_whirl;
	scanhash_functions[ALGO_WHIRLPOOL] = scanhash_whirl;
	scanhash_functions[ALGO_WILDKECCAK] = scanhash_wildkeccak;
	scanhash_functions[ALGO_ZR5] = scanhash_zr5;
}
