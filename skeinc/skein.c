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
    u64b_t X[SKEIN1024_STATE_WORDS];

    // Setup new context
    // IS THIS MEMSET REQUIRED??????????
    //  b must be 0 padded
    memset(&ctx, 0, sizeof(ctx));

    ctx.h.hashBitLen = 1024;
    memcpy(ctx.X, SKEIN1024_IV_1024, sizeof(ctx.X));
    Skein_Start_New_Type(&ctx, MSG);

    // Copy input into buffer (b must be 0 padded - done by memset above)
    memcpy(ctx.b, input, INPUT_SIZE);
    ctx.h.bCnt = INPUT_SIZE;

    // Do final stage
    ctx.h.T[1] |= SKEIN_T1_FLAG_FINAL;                 /* tag as the final block */

    Skein1024_Process_Block(&ctx, ctx.b, 1, ctx.h.bCnt);  /* process the final block */

    /* run Threefish in "counter mode" to generate output */
    memset(ctx.b, 0, sizeof(ctx.b));  /* zero out b[], so it can hold the counter */
    memcpy(X, ctx.X, sizeof(X));      /* keep a local copy of counter mode "key" */

    ((u64b_t *) ctx.b)[0] = 0;          /* build the counter block */
    Skein_Start_New_Type(&ctx, OUT_FINAL);
    Skein1024_Process_Block(&ctx, ctx.b, 1, sizeof(u64b_t)); /* run "counter mode" */

    memcpy(result, ctx.X, SKEIN1024_BLOCK_BYTES);   /* "output" the ctr mode bytes */
}
