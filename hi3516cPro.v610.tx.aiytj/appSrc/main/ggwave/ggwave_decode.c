#include "ggwave.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ggwave_Instance instanceTmp = -1;

int ggwave_instance()
{
    ggwave_Parameters parameters = ggwave_getDefaultParameters();
    parameters.sampleFormatInp = GGWAVE_SAMPLE_FORMAT_I16;
    parameters.sampleFormatOut = GGWAVE_SAMPLE_FORMAT_I16;

    ggwave_rxToggleProtocol(GGWAVE_PROTOCOL_AUDIBLE_FASTEST, 1);
    instanceTmp = ggwave_init(parameters);
    
    return 0;
}

int ggwave_uninstance()
{
    if(instanceTmp != -1) {
        ggwave_free(instanceTmp);
        instanceTmp = -1;
    }
    
    return 0;
}

int ggwave_pcm2str(char *waveform, int waveformlen, char *decoded, int ggwave_param) 
{
    int size = 0;  // size为有效字符串长度
    ggwave_setLogFile(stdout);

    if (ggwave_param == 0) {
        ggwave_uninstance();
        ggwave_instance();
    }
    size = ggwave_decode(instanceTmp, waveform, waveformlen, decoded);
    if (size == 0) {
        return -1;
    } 
    printf("ggwave_decode\n");
    decoded[size] = 0; // null-terminate the received data
    
    return 0;
}



