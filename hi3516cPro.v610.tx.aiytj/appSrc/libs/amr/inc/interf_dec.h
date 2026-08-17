/*
 * ===================================================================
 *  TS 26.104
 *  REL-5 V5.4.0 2004-03
 *  REL-6 V6.1.0 2004-03
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

/*
 * interf_dec.h
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    Defines interface to AMR decoder
 *
 */

#ifndef _interf_dec_h_
#define _interf_dec_h_
#ifdef __cplusplus
extern "C" {
#endif

#include "rom_dec.h"
#include "typedef.h"
#include "decoder_def.h"

/*
 * Function prototypes
 */
/*
 * Conversion from packed bitstream to endoded parameters
 * Decoding parameters to speech
 */
void Decoder_Interface_Decode(sDecoderInterface *st,

#ifndef ETSI
      unsigned char *bits,

#else
      short *bits,
#endif

      short *synth, int bfi );

/*
 * Reserve and init. memory
 */
int Decoder_Interface_init(sDecoderInterface **p_decoder);

/*
 * Exit and free memory
 */
int Decoder_Interface_exit(sDecoderInterface **state);

#ifdef __cplusplus
}
#endif
#endif

