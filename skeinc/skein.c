/***********************************************************************
**
** Implementation of the Skein hash function.
**
** Source code author: Doug Whiting, 2008.
**
** This algorithm and source code is released to the public domain.
**
************************************************************************/

#define  SKEIN_PORT_CODE /* instantiate any code in skein_port.h */

#include <string.h>      /* get the memcpy/memset functions */
#include "skein.h"       /* get the Skein API definitions   */
#include "skein_iv.h"    /* get precomputed IVs */

/* Input size */
#define INPUT_SIZE 0x10

/*****************************************************************/
/* External function to process blkCnt (nonzero) full block(s) of data. */
void Skein1024_Process_Block(Skein1024_Ctxt_t *ctx, const u08b_t *blkPtr, size_t blkCnt, size_t byteCntAdd);

/* Performs the Skein 1024 hash on the given input (input is 16 bytes, output is 1024 bits / 128 bytes) */
void Skein1024_1024_16(const u08b_t *input, u08b_t *result)
{
    Skein1024_Ctxt_t ctx;

    // Setup new context
    memcpy(ctx.X, SKEIN1024_IV_1024, sizeof(ctx.X));
    Skein_Start_New_Type(&ctx, MSG);
    ctx.h.T[1] |= SKEIN_T1_FLAG_FINAL;

    // Copy input into buffer (with zero padding)
    memcpy(ctx.b, input, INPUT_SIZE);
    memset(ctx.b + 16, 0, sizeof(ctx.b) - INPUT_SIZE);

    // Do main hash
    Skein1024_Process_Block(&ctx, ctx.b, 1, INPUT_SIZE);

    // Wipe input buffer and perform final stage
    memset(ctx.b, 0, INPUT_SIZE);

    Skein_Start_New_Type(&ctx, OUT_FINAL);
    Skein1024_Process_Block(&ctx, ctx.b, 1, sizeof(u64b_t));

    // Save result
    memcpy(result, ctx.X, SKEIN1024_BLOCK_BYTES);
}
