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
 * sp_enc.h
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    Defines interface to AMR encoder
 *
 */
#ifndef _SP_DEC_H_
#define _SP_DEC_H_

#include "interf_dec.h"

/*
 * initialize one instance of the speech decoder
 */
int Speech_Decode_Frame_init(sSpeechFrame *s);

/*
 * Decodes one frame from encoded parameters
 */
void Speech_Decode_Frame(sSpeechFrame *st, enum Mode mode, short *serial,
                         enum RXFrameType frame_type, short *synth);

#endif

