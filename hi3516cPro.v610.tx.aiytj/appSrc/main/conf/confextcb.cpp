/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : confextcb.cpp
 * @Created Time : 2014-01-07
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jconfstruct.h"

#include "confextcb.h"


int sysUserMapsCbFunc(int i, void *arg[])
{
    SysUser0 *in = (SysUser0*)arg[0];
    SysUser0 *tmp = (SysUser0*)arg[1];

    memset(tmp, 0x00, sizeof(SysUser0));
    memcpy(tmp, &in[i], sizeof(SysUser0));

    return 0;
}

int sysUserOptsCbFunc(int i, void *arg[])
{
    SysUser0 *in = (SysUser0*)arg[0];
    SysUser0 *tmp = (SysUser0*)arg[1];

    memcpy(&in[i], tmp, sizeof(SysUser0));
    memset(tmp, 0x00, sizeof(SysUser0));

    return 0;
}

int osdExpandMapsCbFunc(int i, void *arg[])
{
    OsdExpand0 *in = (OsdExpand0*)arg[0];
    OsdExpand0 *tmp = (OsdExpand0*)arg[1];

    memset(tmp, 0x00, sizeof(OsdExpand0));
    memcpy(tmp, &in[i], sizeof(OsdExpand0));

    return 0;
}

int osdExpandOptsCbFunc(int i, void *arg[])
{
    OsdExpand0 *in = (OsdExpand0*)arg[0];
    OsdExpand0 *tmp = (OsdExpand0*)arg[1];

    memcpy(&in[i], tmp, sizeof(OsdExpand0));
    memset(tmp, 0x00, sizeof(OsdExpand0));

    return 0;
}

int videoEncMapsCbFunc(int i, void *arg[])
{
    VideoEnc0 *in = (VideoEnc0*)arg[0];
    VideoEnc0 *tmp = (VideoEnc0*)arg[1];

    memset(tmp, 0x00, sizeof(VideoEnc0));
    memcpy(tmp, &in[i], sizeof(VideoEnc0));

    return 0;
}

int videoEncOptsCbFunc(int i, void *arg[])
{
    VideoEnc0 *in = (VideoEnc0*)arg[0];
    VideoEnc0 *tmp = (VideoEnc0*)arg[1];

    memcpy(&in[i], tmp, sizeof(VideoEnc0));
    memset(tmp, 0x00, sizeof(VideoEnc0));

    return 0;
}

int videoMaskMapsCbFunc(int i, void *arg[])
{
    VideoMask0 *in = (VideoMask0*)arg[0];
    VideoMask0 *tmp = (VideoMask0*)arg[1];

    memset(tmp, 0x00, sizeof(VideoMask0));
    memcpy(tmp, &in[i], sizeof(VideoMask0));

    return 0;
}

int videoMaskOptsCbFunc(int i, void *arg[])
{
    VideoMask0 *in = (VideoMask0*)arg[0];
    VideoMask0 *tmp = (VideoMask0*)arg[1];

    memcpy(&in[i], tmp, sizeof(VideoMask0));
    memset(tmp, 0x00, sizeof(VideoMask0));

    return 0;
}

int roiListMapsCbFunc(int i, void *arg[])
{
    RoiArea0 *in = (RoiArea0*)arg[0];
    RoiArea0 *tmp = (RoiArea0*)arg[1];

    memset(tmp, 0x00, sizeof(RoiArea0));
    memcpy(tmp, &in[i], sizeof(RoiArea0));

    return 0;
}

int roiListOptsCbFunc(int i, void *arg[])
{
    RoiArea0 *in = (RoiArea0*)arg[0];
    RoiArea0 *tmp = (RoiArea0*)arg[1];

    memcpy(&in[i], tmp, sizeof(RoiArea0));
    memset(tmp, 0x00, sizeof(RoiArea0));

    return 0;
}

int profileListMapsCbFunc(int i, void *arg[])
{
    ProfileS *in = (ProfileS*)arg[0];
    ProfileS *tmp = (ProfileS*)arg[1];

    memset(tmp, 0x00, sizeof(ProfileS));
    memcpy(tmp, &in[i], sizeof(ProfileS));

    return 0;
}

int profileListOptsCbFunc(int i, void *arg[])
{
    ProfileS *in = (ProfileS*)arg[0];
    ProfileS *tmp = (ProfileS*)arg[1];

    memcpy(&in[i], tmp, sizeof(ProfileS));
    memset(tmp, 0x00, sizeof(ProfileS));

    return 0;
}

int PresetlistMapsCbFunc(int i, void *arg[])
{
    presetlist *in = (presetlist*)arg[0];
    presetlist *tmp = (presetlist*)arg[1];
    memset(tmp, 0x00, sizeof(presetlist));
    memcpy(tmp, &in[i], sizeof(presetlist));
    return 0;
}

int PresetlistOptsCbFunc(int i, void *arg[])
{
    presetlist *in = (presetlist*)arg[0];
    presetlist *tmp = (presetlist*)arg[1];
    memcpy(&in[i], tmp, sizeof(presetlist));
    memset(tmp, 0x00, sizeof(presetlist));
    return 0;
}
