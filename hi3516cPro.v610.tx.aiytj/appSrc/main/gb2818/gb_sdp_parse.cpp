/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_sdp_parse.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 sdp 解析
*/
#include <string.h>

#include <string>

#include "gb_sdp_parse.h"

#include "gb_common.h"

int SdpParse::Decode( const char *body)
{
    // message without sdp
    if( body == NULL ) {
        GB_ERR( "body is NULL\n" );
        return FAILURE;
    }

    const char* p_content = body;
    const char* anchor = 0;

    // 解析SDP字段
    while ( *p_content != '\0' ) {
        char ch = *p_content;
        if ( ch == 'v' ) { // v 字段，基本不用管
            if ( *p_content++ != 'v' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            while ( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) p_content++;

            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }
        } else if ( ch == 'o' ) {  // 会话的所有者或者创建者信息
            if ( *p_content++ != 'o' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            // 用户名
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // p_content - anchor 这一段就是用户名文本，这里不关注
                /*
                std::string recvid(anchor, p_content);
                printf("user name:%s\n", recvid.c_str());
                */
                while( *p_content == ' ' ) p_content++;
                if( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // session id
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // p_content - anchor 这一段会话 ID，这个一般是填 0 ，这里不关注
                /*
                std::string sessionid(anchor, p_content);
                printf("session id:%d\n", atoi(sessionid.c_str()));
                */

                while( *p_content == ' ' ) p_content++;
                if( *p_content == '\0' ) {
                    return FAILURE;
                }
            }

            // version
            anchor = p_content;
            while( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 版本信息，一般填 0 , 这里不关注
                /*
                std::string version(anchor, p_content);
                printf("version:%d\n", atoi(version.c_str()));
                */
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // in
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 这个字段通常是 IN, Internet 的缩写，表示网络类型，直接跳过就行
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // ip version
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 地址类型，通常是 IP4 或者 IP6，直接跳过就行
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // media ip
            anchor = p_content;
            while( *p_content != '\r' && *p_content != '\n' ) p_content++;
            if( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 这个是会话创建者的 IP 地址，通常不关注
                /*
                std::string mediaip(anchor, p_content);
                printf("media ip:%s\n", mediaip.c_str());
                */
            }
        } else if ( ch == 's' ) { // 会话名称
            if ( *p_content++ != 's' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            anchor = p_content;
            while ( *p_content != '\r' && *p_content != '\n' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            // Play Playback Download Talk 标识这个会话的类型
            s_name_.assign(anchor, p_content);
        } else if ( ch == 'u' ) { // uri 这里按简捷方式解析
            if ( *p_content++ != 'u' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            // uri id
            anchor = p_content;
            while ( *p_content != ':' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 源设备 ID 通常不关注
                /*
                std::string id(anchor, p_content);
                printf("id:%s\n", id.c_str());
                */
                p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // uri param
            anchor = p_content;
            while ( *p_content != '\r' && *p_content != '\n' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 相关参数通常不关注
                /*
                std::string param(anchor, p_content);
                printf("param:%s\n", param.c_str());
                */
            }
        } else if ( ch == 'c' ) { // 连接信息，用于指定媒体流的连接地址
            if ( *p_content++ != 'c' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            // in ：Internet 的缩写，通常跳过
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                while( *p_content == ' ' ) p_content++;
                if( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // ip version ：IP4 或者 IP6，目前不支持 IP6 默认跳过
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // connect ip
            anchor = p_content;
            while ( *p_content != '\r' && *p_content != '\n' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // 媒体服务器的 IP 地址，需要保存
                c_ip_.assign( anchor, p_content );
            }
        } else if ( ch == 't' ) { // 用于回放或下载时指定开始和结束时间
            if ( *p_content++ != 't' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            // start time
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                t_starttime_ = atoi(anchor);
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // end time
            anchor = p_content;
            while ( *p_content != '\r' && *p_content != '\n' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                t_endtime_ = atoi(anchor);
            }
        } else if ( ch == 'm' ) { // 描述媒体的媒体类型、端口、传输层协议、负载类型等内容
            if ( *p_content++ != 'm' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            // media type
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                // video audio
                m_mediatype_.assign(anchor, p_content);
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // port
            anchor = p_content;
            while ( *p_content != ' ') p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                m_mediaport_ = atoi(anchor);
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // transport
            anchor = p_content;
            while ( *p_content != ' ' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else { // 传输层协议 TCP/RTP/AVP   RTP/AVP
                m_transport_.assign( anchor, p_content );
                while ( *p_content == ' ' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            // rtpmap 服务器发送的话可能会连着好几个，表示这几种都支持(例如：m=video 30160 RTP/AVP 96 97 98 99)
            // 这里的类型是键，对应的参数信息在 a 字段中，这里先添加键
            while ( *p_content != '\r' && *p_content != '\n' ) {
                anchor = p_content;
                while ( *p_content != ' ' && *p_content != '\r' && *p_content != '\n' ) p_content++;
                if ( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                } else {
                    std::string rtpmap( anchor, p_content );
                    m_rtpmap_.insert( std::make_pair( rtpmap, "" ) );
                    while ( *p_content == ' ' ) p_content++;
                    if ( *p_content == '\0' ) {
                        GB_ERR("The field is wrong!");
                        return FAILURE;
                    }
                }
            }
        } else if( ch == 'b' ) { // 带宽信息，通常跳过
            if ( *p_content++ != 'b' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            while ( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }
        } else if ( ch == 'a' ) { // 属性信息，一个 sdp 中可能会有多个 a=
            if( *p_content++ != 'a' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            std::string type;
            anchor = p_content;
            while( *p_content != ':' && *p_content != '\r' && *p_content != '\n' ) p_content++;
            if( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            } else {
                type.assign( anchor, p_content );
                while( *p_content == ' ' || *p_content == ':' ) p_content++;
                if( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }
            }

            if( strcasecmp( type.c_str(), "rtpmap" ) == 0 ) { // 例子：a=rtpmap:96 PS/90000
                anchor = p_content;
                while( *p_content != ' ' ) p_content++;
                if( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }

                std::string rtpmap( anchor, p_content );
                while( *p_content == ' ' ) p_content++;
                if( *p_content == '\0' ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }

                std::map< std::string, std::string >::iterator it = m_rtpmap_.find( rtpmap );
                if( it == m_rtpmap_.end() ) {
                    GB_ERR("The field is wrong!");
                    return FAILURE;
                }

                anchor = p_content;
                while( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                    p_content++;
                }

                if( anchor != p_content ) {
                    it->second.assign( anchor, p_content );
                }
            } else if ( strcasecmp( type.c_str(), "setup" ) == 0 ) { // a=setup:passive
                anchor = p_content;
                while( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                    p_content++;
                }

                if( *p_content != '\0' ) {
                    a_setup_.assign( anchor, p_content );
                }
            } else if ( strcasecmp( type.c_str(), "downloadspeed" ) == 0 ) {
                anchor = p_content;
                while( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                    p_content++;
                }

                if( *p_content != '\0' ) {
                    a_download_speed_ = atof(anchor);
                }
            } else if (strcasecmp( type.c_str(), "connection" ) == 0) {
                anchor = p_content;
                while( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                    p_content++;
                }

                if( *p_content != '\0' ) {
                    a_connect_.assign( anchor, p_content );
                }
            } else {
                // a=recvonly a=sendonly a=sendrecv a=inactive
                // a=filesize:
                // 这些属性暂时没什么用，先不关注
                anchor = p_content;
                while( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                    p_content++;
                }
            }
        } else if( ch == 'y' ) { // ssrc 字段
            if ( *p_content++ != 'y' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            anchor = p_content;
            while ( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                p_content++;
            }

            if ( anchor != p_content ) {
                y_ssrc_ = atoi(anchor);
            }
        } else if( ch == 'f' ) { // 编码属性
            if ( *p_content++ != 'f' || *p_content++ != '=' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }

            anchor = p_content;
            while ( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) {
                p_content++;
            }

            if ( anchor != p_content ) {
                f_encode_proterty_.assign( anchor, p_content );
            }
        } else {
            while ( *p_content != '\0' && *p_content != '\r' && *p_content != '\n' ) p_content++;
            if ( *p_content == '\0' ) {
                GB_ERR("The field is wrong!");
                return FAILURE;
            }
        }

        // next line
        while ( *p_content == '\r' || *p_content == '\n' ) p_content++;
        if ( *p_content == '\0' ) {
            break;
        }
    }

    PrintfAll(); // 参数打印一遍

    return SUCCESS;
}

void SdpParse::PrintfAll()
{
    GB_INFO("name:%s\n", s_name_.c_str());
    GB_INFO("ip address:%s\n", c_ip_.c_str());
    GB_INFO("start time:%lld\n", t_starttime_);
    GB_INFO("end time:%lld\n", t_endtime_);
    GB_INFO("media type:%s\n", m_mediatype_.c_str());
    GB_INFO("media port:%u\n", m_mediaport_);
    GB_INFO("transfer Protocol:%s\n", m_transport_.c_str());

    std::map< std::string, std::string >::iterator it = m_rtpmap_.begin();
    for (; it != m_rtpmap_.end(); ++it) {
        GB_INFO("rtpmap:%s %s\n", it->first.c_str(), it->second.c_str());
    }

    GB_INFO("setup:%s\n", a_setup_.c_str());
    GB_INFO("ssrc:%010u\n", y_ssrc_);
    GB_INFO("encode proterty:%s\n", f_encode_proterty_.c_str());
}

/*
    video_fps 单位:帧/s
    video_bps 单位:kbps
    audio_bps 单位:bps
    audio_sample_rate 单位:bps
*/
int SdpParse::ParseEncodeParam(GbVideoCodecType &video_codec, GbResolution &video_venc, int &video_fps, GbBitrateControl &bitrate_ctrl, int &video_bps, GbAudioCodecType &audio_codec, int &audio_bps, int &audio_sample_rate)
{
    // f= v/编码格式/分辨率/帧率/码率类型/码率大小a/编码格式/码率大小/采样率
    // 例子 f=v/2/5/20/1/512a/1/8/1
    // 如果参数没有则分割符直接相连

    // 可能会不存在 f 值，先赋值默认参数
    video_codec = GB_H264;
    video_venc = GB_1080P;
    video_fps = 25;
    bitrate_ctrl = GB_VBR;
    video_bps = 100;
    audio_codec = GB_G711;
    audio_bps = 64000;
    audio_sample_rate = 8000;

    if (f_encode_proterty_.size() == 0) {
        return SUCCESS;
    }

    const char *p1 = NULL;
    p1 = f_encode_proterty_.c_str();

    do {
        int unmber = 0;
        // "v/"" 开头
        if (p1[0] != 'v' || p1[1] != '/') {
            break;
        }

        p1 += 2;

        // 编码格式
        unmber = atoi(p1);
        if (unmber > GB_VIDEO_CODEC_UNKNOWN && unmber < GB_VIDEO_CODEC_UNMBER) {
            video_codec = (GbVideoCodecType)unmber;
        } else {
            video_codec = GB_VIDEO_CODEC_UNKNOWN;
        }
        GB_INFO("video_codec:%d\n", unmber);

        p1 = strchr(p1, '/');
        if (p1 == NULL)
            break;
        p1++;

        // 分辨率
        unmber = atoi(p1);
        if (unmber > GB_RESOLUTION_UNKNOWN && unmber < GB_RESOLUTION_UNMBER) {
            video_venc = (GbResolution)unmber;
        } else {
            video_venc = GB_1080P; // 默认推主码流
        }
        GB_INFO("video venc:%d\n", unmber);

        p1 = strchr(p1, '/');
        if (p1 == NULL)
            break;
        p1++;

        // 帧率
        video_fps = atoi(p1);
        GB_INFO("video fps:%d\n", video_fps);

        p1 = strchr(p1, '/');
        if (p1 == NULL)
            break;
        p1++;

        // 码率类型
        unmber = atoi(p1);
        if (unmber > GB_BITRATE_CONTROL_UNKNOWN && unmber < GB_BITRATE_CONTROL_UNMBER) {
            bitrate_ctrl = (GbBitrateControl)unmber;
        } else {
            bitrate_ctrl = GB_BITRATE_CONTROL_UNKNOWN;
        }
        GB_INFO("fix bps:%d\n", unmber);

        p1 = strchr(p1, '/');
        if (p1 == NULL)
            break;
        p1++;

        // 码率大小
        video_bps = atoi(p1);
        GB_INFO("video bps:%d\n", video_bps);

        // 检测音频参数
        p1 = strchr(p1, '/');
        if (p1 == NULL || *(p1 - 1) != 'a')
            break;
        p1++;

        // 音频编码格式
        unmber = atoi(p1);
        if (unmber > GB_AUDIO_CODEC_UNKNOWN && unmber < GB_AUDIO_CODEC_UNMBER) {
            audio_codec = (GbAudioCodecType)unmber;
        } else {
            audio_codec = GB_AUDIO_CODEC_UNKNOWN;
        }
        GB_INFO("audio enc:%d\n", unmber);

        p1 = strchr(p1, '/');
        if (p1 == NULL)
            break;
        p1++;

        // 音频编码码率
        unmber = atoi(p1);
        if (unmber == 1)
            audio_bps = 5300; // 5.3 kbps
        else if (unmber == 2)
            audio_bps = 6300;
        else if (unmber >= 3 && unmber <= 6)
            audio_bps = 8000 * (unmber - 2);
        else if (unmber == 7)
            audio_bps = 48000;
        else if (unmber == 8)
            audio_bps = 64000;
        GB_INFO("audio bps:%d\n", unmber);

        p1 = strchr(p1, '/');
        if (p1 == NULL)
            break;
        p1++;

        // 采样率
        unmber = atoi(p1);
        if (unmber == 1)
            audio_sample_rate = 8000;
        else if (unmber == 2)
            audio_sample_rate = 14000;
        else if (unmber == 3)
            audio_sample_rate = 16000;
        else if (unmber == 4)
            audio_sample_rate = 32000;
        GB_INFO("audio sample rate:%d\n", unmber);

        GB_INFO("video_codec:%d video_venc:%d video_fps:%d bitrate_ctrl:%d video_bps:%d audio_codec:%d audio_bps:%d audio_sample_rate:%d\n"
                    , video_codec, video_venc, video_fps, bitrate_ctrl, video_bps, audio_codec, audio_bps, audio_sample_rate);

        return SUCCESS;
    } while (0);


    GB_ERR("parse fail, f=%s\n", f_encode_proterty_.c_str());

    return FAILURE;
}

GbVideoCodecType SdpParse::CodecToVideoFormat(VENC_FORMAT_E codec)
{
    if (codec == VENC_FORMAT_H265) {
        return GB_H265;
    } else if (codec == VENC_FORMAT_H264) {
        return GB_H264;
    } else if (codec == VENC_FORMAT_SVC) {
        return GB_SVAC;
    }

    return GB_H265;
}

VENC_FORMAT_E SdpParse::VideoFormatToCodec(GbVideoCodecType gb_codec)
{
    if (gb_codec == GB_H265) {
        return VENC_FORMAT_H265;
    } else if (gb_codec == GB_H264) {
        return VENC_FORMAT_H264;
    } else if (gb_codec == GB_SVAC) {
        return VENC_FORMAT_SVC;
    }

    return VENC_FORMAT_BEGIN;
}

const char* SdpParse::VencsizeToResolution(VencSizeE vencsize)
{
    const char *res = NULL;
    // 分辨率如 GbResolution 标注的，除了几个有规定的其他用宽x高表示
    switch (vencsize) {
        case VencSizeE_QCIF:
            res = "1";
            break;
        case VencSizeE_CIF:
            res = "2";
            break;
        case VencSizeE_D1:
            res = "4";
            break;
        case VencSizeE_720P:
            res = "5";
            break;
        case VencSizeE_UVGA:
            res = "1600×1200";
            break;
        case VencSizeE_1080P:
            res = "6";
            break;
        case VencSizeE_QVGA:
            res = "320×240";
            break;
        case VencSizeE_VGA:
            res = "640×480";
            break;
        case VencSizeE_960P:
            res = "1280×960";
            break;
        case VencSizeE_3M:
            res = "2048×1536";
            break;
        case VencSizeE_180P:
            res = "320×180";
            break;
        case VencSizeE_360P:
            res = "640×360";
            break;
        case VencSizeE_4M:
            res = "2272×1704";
            break;
        case VencSizeE_5M:
            res = "2592×1944";
            break;
        default:
            GB_ERR("not find vencsize\n");
            res = "6";
            break;
    }

    return res;
}

VencSizeE SdpParse::ResolutionToVencsize(const char *str)
{
    VencSizeE venc_size = VencSizeE_BEGIN;
    if (strcmp(str, "1") == 0) {
        venc_size = VencSizeE_QCIF;
    } else if (strcmp(str, "2") == 0) {
        venc_size = VencSizeE_CIF;
    } else if (strcmp(str, "4") == 0) {
        venc_size = VencSizeE_D1;
    } else if (strcmp(str, "5") == 0) {
        venc_size = VencSizeE_720P;
    } else if (strcmp(str, "1600×1200") == 0) {
        venc_size = VencSizeE_UVGA;
    } else if (strcmp(str, "6") == 0) {
        venc_size = VencSizeE_1080P;
    } else if (strcmp(str, "320×240") == 0) {
        venc_size = VencSizeE_QVGA;
    } else if (strcmp(str, "640×480") == 0) {
        venc_size = VencSizeE_VGA;
    } else if (strcmp(str, "1280×960") == 0) {
        venc_size = VencSizeE_960P;
    } else if (strcmp(str, "2048×1536") == 0) {
        venc_size = VencSizeE_3M;
    } else if (strcmp(str, "320×180") == 0) {
        venc_size = VencSizeE_180P;
    } else if (strcmp(str, "640×360") == 0) {
        venc_size = VencSizeE_360P;
    } else if (strcmp(str, "2272×1704") == 0) {
        venc_size = VencSizeE_4M;
    } else if (strcmp(str, "2592×1944") == 0) {
        venc_size = VencSizeE_4M;
    } else {
        GB_ERR("not find vencsize:%s\n", str);
        venc_size = VencSizeE_1080P;
    }

    return venc_size;
}

GbBitrateControl SdpParse::FixbpsToBitRateType(int fixbps)
{
    // fixbps :1 定码流，0 变码流
    if (fixbps == 1) {
        return GB_CBR;
    } else if (fixbps == 0) {
        return GB_VBR;
    }

    return GB_VBR;
}

int SdpParse::BitRateTypeToFixbps(GbBitrateControl bit_ctr)
{
    if (bit_ctr == GB_CBR) {
        return 1;
    } else if (bit_ctr == GB_VBR) {
        return 0;
    }

    return 0;
}