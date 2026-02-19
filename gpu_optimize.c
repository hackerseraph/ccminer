// GPU optimization implementations
#include "gpu_optimize.h"
#include "algos.h"
#include <stdio.h>

lock_free_log_t gpu_log_buffer = {0};
gpu_occupancy_t gpu_occupancy[100] = {0};

// Forward declarations of all scanhash functions
extern int scanhash_allium(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_bastion(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_blake256(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, int rounds);
extern int scanhash_blake2b(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_blake2s(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_bmw(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_c11(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_cryptolight(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, int variant);
extern int scanhash_cryptonight(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, int variant);
extern int scanhash_decred(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_deep(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_equihash(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_fresh(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_fugue256(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_groestlcoin(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_myriad(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_hmq17(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_hsr(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_heavy(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, int vote, int header_size);
extern int scanhash_keccak256(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_jackpot(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_jha(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_lbry(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_luffa(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_quark(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_qubit(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_lyra2(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_lyra2v2(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_lyra2v3(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_lyra2Z(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_neoscrypt(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_nist5(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_pentablake(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_phi(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_phi2(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_polytimos(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_scrypt(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, void *scratchpad, struct timeval *tv_start, struct timeval *tv_end);
extern int scanhash_scrypt_jane(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, void *scratchpad, struct timeval *tv_start, struct timeval *tv_end);
extern int scanhash_skeincoin(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_skein2(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_skunk(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_sha256d(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_sha256t(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_sha256q(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_sia(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_sib(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_sonoa(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_s3(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_vanilla(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done, int rounds);
extern int scanhash_veltor(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_whirl(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_wildkeccak(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_timetravel(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_tribus(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_bitcore(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_exosis(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x11evo(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x11(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x12(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x13(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x14(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x15(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x16r(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x16s(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_x17(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);
extern int scanhash_zr5(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done);

// Function pointer array indexed by algorithm ID
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
