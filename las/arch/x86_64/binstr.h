/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _X86_64_BINSTR_H
#define _X86_64_BINSTR_H

#include "../../las.h"
#include "reg.h"
#include "eval.h"
#include <stdlib.h>
#include <string.h>

typedef enum _x86_64_enc {
    ENC_NP,
    ENC_NP_AL_DX,
    ENC_NP_AX_DX,
    ENC_NP_EAX_DX,
    ENC_NP_DX_AL,
    ENC_NP_DX_AX,
    ENC_NP_DX_EAX,
    ENC_I_IMM16,
    ENC_I_AL_IMM8,
    ENC_I_AX_IMM8,
    ENC_I_AX_IMM16,
    ENC_I_EAX_IMM8,
    ENC_I_EAX_IMM32,
    ENC_I_RAX_IMM32,
    ENC_I_IMM8_AL,
    ENC_I_IMM8_AX,
    ENC_I_IMM8_EAX,
    ENC_MI_RM8_IMM8,
    ENC_MI_RM16_IMM8,
    ENC_MI_RM16_IMM16,
    ENC_MI_RM32_IMM8,
    ENC_MI_RM32_IMM32,
    ENC_MI_RM64_IMM8,
    ENC_MI_RM64_IMM32,
    ENC_MI_RM64_IMM64,
    ENC_MR_RM8_R8,
    ENC_MR_RM16_R16,
    ENC_MR_RM32_R32,
    ENC_MR_RM64_R64,
    ENC_RM_R8_RM8,
    ENC_RM_R16_RM16,
    ENC_RM_R32_RM8,
    ENC_RM_R32_RM16,
    ENC_RM_R32_RM32,
    ENC_RM_R64_RM8,
    ENC_RM_R64_RM64,
    ENC_O_R32,
    ENC_O_R64,
    ENC_OI_R8_IMM8,
    ENC_OI_R16_IMM16,
    ENC_OI_R32_IMM32,
    ENC_OI_R64_IMM64,
    ENC_M_M8,
    ENC_M_RM8,
    ENC_M_RM16,
    ENC_M_RM32,
    ENC_M_RM64,
    ENC_RMI_R16_RM16_IMM8,
    ENC_RMI_R16_RM16_IMM16,
    ENC_RMI_R32_RM32_IMM8,
    ENC_RMI_R32_RM32_IMM32,
    ENC_RMI_R64_RM64_IMM8,
    ENC_RMI_R64_RM64_IMM32,
} x86_64_enc_t;

#ifdef __cplusplus
extern "C" {
#endif

    int
    binstr(x86_64_instr_t *, x86_64_target_t, int, int, int, int, int,
           const operand_vector_t *, x86_64_enc_t);

#ifdef __cplusplus
}
#endif

#endif /* _X86_64_BINSTR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
