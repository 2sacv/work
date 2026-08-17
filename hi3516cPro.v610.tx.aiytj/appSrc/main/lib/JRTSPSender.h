/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : 
 * Created Time : 2014-06-11
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _JSender_H_
#define _JSender_H_

class JRTSPSender
{
public:
	JRTSPSender(int fd);
	~JRTSPSender();

	int WriteCMDData(char *cmd, int cmdlen);
	int WriteRTPData(char * packet, int packetSize, unsigned char ChannelId, int needBack);
	int Flush();
	int	GetBackDataSize();

private:
	int fFileDesc;
	int	fBytesSentInBuffer;

	void* fBacker;
	
};

#endif
