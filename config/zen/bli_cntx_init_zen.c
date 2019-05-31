/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin
   Copyright (C) 2018 - 2019, Advanced Micro Devices, Inc.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name(s) of the copyright holder(s) nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"

void bli_cntx_init_zen( cntx_t* cntx )
{
	blksz_t blkszs[ BLIS_NUM_BLKSZS ];

	// Set default kernel blocksizes and functions.
	bli_cntx_init_zen_ref( cntx );

	// -------------------------------------------------------------------------

	// Update the context with optimized native gemm micro-kernels and
	// their storage preferences.
	bli_cntx_set_l3_nat_ukrs
	(
	  8,
	  // gemm
	  BLIS_GEMM_UKR,       BLIS_FLOAT,    bli_sgemm_haswell_asm_6x16,       TRUE,
	  BLIS_GEMM_UKR,       BLIS_DOUBLE,   bli_dgemm_haswell_asm_6x8,        TRUE,
	  BLIS_GEMM_UKR,       BLIS_SCOMPLEX, bli_cgemm_haswell_asm_3x8,        TRUE,
	  BLIS_GEMM_UKR,       BLIS_DCOMPLEX, bli_zgemm_haswell_asm_3x4,        TRUE,
	  // gemmtrsm_l
	  BLIS_GEMMTRSM_L_UKR, BLIS_FLOAT,    bli_sgemmtrsm_l_haswell_asm_6x16, TRUE,
	  BLIS_GEMMTRSM_L_UKR, BLIS_DOUBLE,   bli_dgemmtrsm_l_haswell_asm_6x8,  TRUE,
	  // gemmtrsm_u
	  BLIS_GEMMTRSM_U_UKR, BLIS_FLOAT,    bli_sgemmtrsm_u_haswell_asm_6x16, TRUE,
	  BLIS_GEMMTRSM_U_UKR, BLIS_DOUBLE,   bli_dgemmtrsm_u_haswell_asm_6x8,  TRUE,
	  cntx
	);

	// Update the context with optimized level-1f kernels.
	bli_cntx_set_l1f_kers
	(
	  4,
	  // axpyf
	  BLIS_AXPYF_KER,     BLIS_FLOAT,  bli_saxpyf_zen_int_8,
	  BLIS_AXPYF_KER,     BLIS_DOUBLE, bli_daxpyf_zen_int_8,
	  // dotxf
	  BLIS_DOTXF_KER,     BLIS_FLOAT,  bli_sdotxf_zen_int_8,
	  BLIS_DOTXF_KER,     BLIS_DOUBLE, bli_ddotxf_zen_int_8,
	  cntx
	);

	// Update the context with optimized level-1v kernels.
	bli_cntx_set_l1v_kers
	(
	  10,
	  // amaxv
	  BLIS_AMAXV_KER,  BLIS_FLOAT,  bli_samaxv_zen_int,
	  BLIS_AMAXV_KER,  BLIS_DOUBLE, bli_damaxv_zen_int,
	  // axpyv
#if 0
	  BLIS_AXPYV_KER,  BLIS_FLOAT,  bli_saxpyv_zen_int,
	  BLIS_AXPYV_KER,  BLIS_DOUBLE, bli_daxpyv_zen_int,
#else
	  BLIS_AXPYV_KER,  BLIS_FLOAT,  bli_saxpyv_zen_int10,
	  BLIS_AXPYV_KER,  BLIS_DOUBLE, bli_daxpyv_zen_int10,
#endif
	  // dotv
	  BLIS_DOTV_KER,   BLIS_FLOAT,  bli_sdotv_zen_int,
	  BLIS_DOTV_KER,   BLIS_DOUBLE, bli_ddotv_zen_int,
	  // dotxv
	  BLIS_DOTXV_KER,  BLIS_FLOAT,  bli_sdotxv_zen_int,
	  BLIS_DOTXV_KER,  BLIS_DOUBLE, bli_ddotxv_zen_int,
	  // scalv
#if 0
	  BLIS_SCALV_KER,  BLIS_FLOAT,  bli_sscalv_zen_int,
	  BLIS_SCALV_KER,  BLIS_DOUBLE, bli_dscalv_zen_int,
#else
	  BLIS_SCALV_KER,  BLIS_FLOAT,  bli_sscalv_zen_int10,
	  BLIS_SCALV_KER,  BLIS_DOUBLE, bli_dscalv_zen_int10,
#endif
	  cntx
	);

	// Initialize level-3 blocksize objects with architecture-specific values.
	//                                           s      d      c      z
	bli_blksz_init_easy( &blkszs[ BLIS_MR ],     6,     6,     3,     3 );
	bli_blksz_init_easy( &blkszs[ BLIS_NR ],    16,     8,     8,     4 );

/*
	Multi Instance performance improvement of DGEMM when binded to a CCX
	In Multi instance each thread runs a sequential DGEMM.

	a) 	If BLIS is run in a multi instance mode with 
		CPU freq 2.6/2.2 Ghz
		DDR4 clock frequency 2400Mhz
                mc = 240, kc = 512, and nc = 2040
		has better performance on EPYC server, over the default block sizes.

	b)  	If BLIS is run in Single Instance mode 
		mc = 510, kc = 1024 and nc = 4080

*/

      // Zen optmized level 3 cache block sizes
#ifdef BLIS_ENABLE_ZEN_BLOCK_SIZES

   #if BLIS_ENABLE_SINGLE_INSTANCE_BLOCK_SIZES
  
        bli_blksz_init_easy( &blkszs[ BLIS_MC ],   144,  510,   144,    72 );
        bli_blksz_init_easy( &blkszs[ BLIS_KC ],   256,  1024,   256,   256 );
        bli_blksz_init_easy( &blkszs[ BLIS_NC ],  4080,  4080,  4080,  4080 );

   #else

        bli_blksz_init_easy( &blkszs[ BLIS_MC ],   144,   240,   144,    72 );
        bli_blksz_init_easy( &blkszs[ BLIS_KC ],   256,   512,   256,   256 );
        bli_blksz_init_easy( &blkszs[ BLIS_NC ],  4080,   2040,  4080,  4080 );

   #endif

	bli_blksz_init_easy( &blkszs[ BLIS_MC ],   144,   72,   144,    72 );
        bli_blksz_init_easy( &blkszs[ BLIS_KC ],   256,   256,   256,   256 );
        bli_blksz_init_easy( &blkszs[ BLIS_NC ],  4080,   4080,  4080,  4080 );

#endif



	bli_blksz_init_easy( &blkszs[ BLIS_AF ],     8,     8,    -1,    -1 );
	bli_blksz_init_easy( &blkszs[ BLIS_DF ],     8,     8,    -1,    -1 );

	// Update the context with the current architecture's register and cache
	// blocksizes (and multiples) for native execution.
	bli_cntx_set_blkszs
	(
	  BLIS_NAT, 7,
	  // level-3
	  BLIS_NC, &blkszs[ BLIS_NC ], BLIS_NR,
	  BLIS_KC, &blkszs[ BLIS_KC ], BLIS_KR,
	  BLIS_MC, &blkszs[ BLIS_MC ], BLIS_MR,
	  BLIS_NR, &blkszs[ BLIS_NR ], BLIS_NR,
	  BLIS_MR, &blkszs[ BLIS_MR ], BLIS_MR,
	  // level-1f
	  BLIS_AF, &blkszs[ BLIS_AF ], BLIS_AF,
	  BLIS_DF, &blkszs[ BLIS_DF ], BLIS_DF,
	  cntx
	);
}

