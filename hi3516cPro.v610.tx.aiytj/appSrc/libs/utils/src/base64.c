#include <stdint.h>
#include "base64.h"
#include "debug.h"

static const char *Base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// 填充表：用于根据输入数据的长度决定需要填充的 `=` 数量
static const int mod_table[] = {0, 2, 1};
static int b64_decode_table[256] =
{
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};


int base64encode(char *dst, int *dstlen, char *src, int srclen)
{
    if (NULL == dst || NULL == dstlen || NULL == src)
    {
        return FAILURE;
    }

    int input_length = srclen;
    int output_length = 4 * ((input_length + 2) / 3);

    if (output_length > *dstlen) {
        ERR("dstlen too small\n");
        return FAILURE;
    }

    // 进行 Base64 编码
    for (int i = 0, j = 0; i < input_length;) {
        // 每次处理3个字节
        uint32_t octet_a = i < input_length ? (unsigned char)src[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)src[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)src[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        // 把 3 个字节转为 4 个 Base64 字符
        dst[j++] = Base64Table[(triple >> 3 * 6) & 0x3F];
        dst[j++] = Base64Table[(triple >> 2 * 6) & 0x3F];
        dst[j++] = Base64Table[(triple >> 1 * 6) & 0x3F];
        dst[j++] = Base64Table[(triple >> 0 * 6) & 0x3F];
    }

    // 根据输入长度添加填充字符 '='
    for (int i = 0; i < mod_table[input_length % 3]; i++) {
        dst[output_length - 1 - i] = '=';
    }

    // 添加字符串结束符
    dst[output_length] = '\0';
    *dstlen = output_length;

	return SUCCESS;
}

int base64decode (char *dst, int *dstlen, char *src, int srclen)
{
	const char* cp;
	int space_idx, phase;
	int d, prev_d = 0;
	unsigned char c;
	
	if (NULL == dst || NULL == dstlen || NULL == src)
	{
		return FAILURE;
	}
	
	space_idx = 0;
	phase = 0;
	for (cp = src; *cp != '\0'; ++cp)
	{
		d = b64_decode_table[(int) *cp];
		if (d != -1)
		{
			switch (phase)
			{
				case 0:
					++phase;
					break;
				case 1:
					c = ((prev_d << 2) | ((d & 0x30) >> 4));
					if (space_idx < srclen)
						dst[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = (((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
					if (space_idx < srclen)
						dst[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = (((prev_d & 0x03) << 6) | d);
					if (space_idx < srclen)
						dst[space_idx++] = c;
					phase = 0;
					break;
			}
			
			prev_d = d;
		}
	}
	
	*dstlen = space_idx;
	return SUCCESS;
}



