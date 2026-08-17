#ifndef __BASE_64__H___
#define __BASE_64__H___

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================
 discrib: Base64 Encoder
 	The input data is arbitrary binary data, the output is a \0 terminated string.
 	src and dst may not share the same buffer.
 param: 
	int base64encode(		-OUT SUCCESS/FAILURE
			char *dst,		-OUT encoded data
			int *dstlen,		-OUT dst data length
			const void *src,	-IN source data
			int srclen)		-IN src data length
=====================================================================*/ 

int base64encode(char *dst, int *dstlen, char *src, int srclen);

/*====================================================================
 discrib: Base64 Decoder
	Do base-64 decoding on a string.  Ignore any non-base64 bytes.
	Return the actual number of bytes generated.  The decoded size will be at most 3/4 the size of the encoded,
	and may be smaller if there are padding characters (blanks, newlines).
 param: 
	int base64decode(		-OUT SUCCESS/FAILURE
			char *dst,		-OUT deencoded data
			int *dstlen,		-OUT dst data length
			const void *src,	-IN source encrypted data
			int srclen) 		-IN src data length
=====================================================================*/ 

int base64decode (char *dst, int *dstlen, char *src, int srclen);


#ifdef __cplusplus
}
#endif

#endif

