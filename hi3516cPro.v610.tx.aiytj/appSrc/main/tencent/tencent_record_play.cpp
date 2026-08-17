#ifdef PLATFORM_TENCENT
#include "g711.h"
#include "utils.h"
#include "debug.h"
#include "confapi.h"
#include "shm_buf.h"
#include "js_scheduler.h"
#include "jconfstruct.h"
#include "encode_common.h"
#include "encode_aac.h"

#include "record_lib.h"
#include "tencent_record_play.h"
#include "tencent_param_conf.h"
#include "tencent_server.h"
#include "tencent_media_manage.h"
#include "tencent_talk.h"
#include "tencent_living_stream.h"

#define REC_VIDEO_BUF_SIZE    MAX_FRAME_BYTES
#define REC_AUDIO_BUF_SIZE    1280              // aac 编码不均匀，可高达 PCM 数据量的 90%

#define MAX_REC_FILENAME_LEN  64
#define REAL_REC_FILENAME_LEN 17

std::vector<RecordPlayChnS*> g_replay;

static int replay_init = 0;

// H.264 NAL 单元类型定义
#define NAL_TYPE_AVC_SEI 6
// H.265/HEVC NAL 单元类型定义
#define NAL_TYPE_HEVC_SEI 39

// 构造 SEI 数据(NAL 单元)
int construct_sei(int video_type, uint8_t *sei_buffer, int buffer_size, const char * sei_timestamp)
{

    size_t timestamp_length = strlen(sei_timestamp);

    uint8_t uuid[16] = {
        0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF
    };

    int pos = 0;

    // NAL 起始码(0x00 00 00 01)
    sei_buffer[pos++] = 0x00;
    sei_buffer[pos++] = 0x00;
    sei_buffer[pos++] = 0x00;
    sei_buffer[pos++] = 0x01;

    if (video_type == VIDEO_H265) {
        // NAL 单元头部（NAL 类型 39：SEI）
        sei_buffer[pos++] = (NAL_TYPE_HEVC_SEI << 1); // 高位6位表示NAL单元类型
        sei_buffer[pos++] = 0x01; // 低位1位固定为1（对于HEVC的NAL头部）
    } else if (video_type == VIDEO_H264) {
        // NAL 单元头部(NAL 类型 6：SEI)
        sei_buffer[pos++] = (NAL_TYPE_AVC_SEI & 0x1F);
    }
    sei_buffer[pos++] = 0x05; // SEI 消息类型
    sei_buffer[pos++] = 0x1A + timestamp_length; // payload size
    // UUID (16 bytes)
    memcpy(&sei_buffer[pos], uuid, sizeof(uuid));
    pos += sizeof(uuid);

    // 时间戳 (10 bytes)
    memcpy(&sei_buffer[pos], sei_timestamp, timestamp_length);
    pos += timestamp_length;
    return pos; // 返回 SEI 数据的长度
}

int is_replay_chn_full(void)
{
    return (g_replay.size() == TENCENT_REPLAY_MAX_CHN);
}

static int replay_get_paly_chn(int id)
{
    unsigned int i = 0;
    for (i = 0; i < g_replay.size(); i++) {
        if (id == g_replay[i]->mng.id) {
            DBG("the param is exist, id:%d, id:%d\n", id, g_replay[i]->mng.id);
            return i;
        }
    }
    DBG("id:%d the param is not exist\n", id);

    return -1;
}

int is_replay_chn_running(int id)
{
    if (replay_get_paly_chn(id) >= 0) {
        return TRUE;
    }

    return FALSE;
}

/*
 * 录像查询二分查找，及打印优化
 **/
int replay_find_record_file(RecordInfoS *info, char *file_path, int path_len)
{
    static int tick = 0;
    unsigned int seek_epoch = info->start_time_s + info->seek_timestamp_ms / 1000;
    int left = 0;
    int right = info->file_count - 1;
    int target_index = -1;

    if (is_inc_modc(tick, 2)) {
        DropCache(__func__);
        CompactMemo(__func__);
    }

    // 二分查找核心逻辑[3,7]
    while (left <= right) {
        int mid = left + (right - left) / 2;
        sRec1File* current = &info->file_list[mid];

        if (seek_epoch >= current->start_time) {
            // 候选文件可能在右侧或当前位置
            if (seek_epoch < current->stop_time && 
                (seek_epoch - current->start_time) <= current->file_secs) {
                target_index = mid;
                break;
            }
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    // 未找到尽量左右靠边
    if (target_index == -1) {
        // 后续无录像,找最后一个已关闭的非9999录像
        if (seek_epoch == info->file_list[info->file_count - 1].stop_time) {
            target_index = info->file_count - 1;
        }
        // 后续有录像,有空白,找最新9999录像
        else if (right == info->file_count - 1) {
            target_index = info->file_count - 1;
        } else {
            return -1;
        }
    }

    // 生成文件路径并记录日志
    sRec1File* target_file = &info->file_list[target_index];
    snprintf(file_path, path_len, "%s%s", info->file_dir, target_file->file_name);
    info->play_num = target_index;

    WAR("total:[%d/%d] %s seek_00am_off_secs:%u "
        "seek_epoch:%u, file_secs:%d\n", 
        target_index, info->file_count, target_file->file_name, info->seek_timestamp_ms/1000,
        seek_epoch,  target_file->file_secs);

    return 0;
}

int replay_find_record_list(RecordInfoS* info, int channel)
{
    int ret = -1;
    int file_num = 0;
    int yyyymmdd = 0;
    char dir_path[64] = {0};
    sRec1File *pre_reclist = NULL;
    sRec1File *real_reclist = NULL;

    do {
        pre_reclist = (sRec1File *)calloc(MAX_RECS_OF_DAY, sizeof(sRec1File));
        if (pre_reclist == NULL) {
            ERR("malloc pre reclist failed\n");
            return ret;
        }

        yyyymmdd = record_get_ymd_of_epoch(info->start_time_s);
        if (record_get_path_of_ymd(yyyymmdd, dir_path, sizeof(dir_path), channel) < 0) {
            ERR("%s tencent get record path dir failed%s\n", "\033[1;31m", "\033[0m");
            break;
        }

        file_num = record_query_list(info->start_time_s, MAX_RECS_OF_DAY, pre_reclist, channel);
        if (file_num <= 0) {
            ERR("%s tencent player not find file %s\n", "\033[1;31m", "\033[0m");
            break;
        }

        real_reclist = (sRec1File*)calloc(file_num, sizeof(sRec1File));
        if (real_reclist == NULL) {
            ERR("malloc real reclist failed\n");
            break;
        }

        memcpy(info->file_dir, dir_path, sizeof(dir_path));
        memcpy(real_reclist , pre_reclist, file_num*sizeof(sRec1File));
        info->file_list = real_reclist;
        info->file_count = file_num;
        ret = 0;
    } while(0);

    if (NULL != pre_reclist) {
        free(pre_reclist);
        pre_reclist = NULL;
    }

    return ret;
}

int replay_open_record_file(RecordPlayChnS* player, char* file_path)
{
    int ret = -1;
    int video_type = 0;
    int offset_time = -1;
    double video_fps = 0;
    bool audio_exist = FALSE;
    unsigned int audio_fps = 0;
    tagMP4AudioInfo audio_info;
    tagMP4VideoInfo video_info;
    CMP4Read * pCMP4Read = NULL;
    RecordInfoS* rec_info = &player->rec_info;

	do {
		offset_time = rec_info->start_time_s+rec_info->seek_timestamp_ms/1000 - rec_info->file_list[rec_info->play_num].start_time;
		/*if (offset_time >= 3) {
			rec_info->seek_timestamp_ms -= 3000;
			offset_time -= 3;
			DBG("seek_timestamp_ms: %u, offset_time: %d\n", rec_info->seek_timestamp_ms, offset_time);
		}*/

        pCMP4Read = record_open_file(file_path, &audio_exist, &audio_fps, &audio_info, &video_type, &video_fps, &video_info);
        if (pCMP4Read == NULL) {
            ERR("tencent player fail to open record file:%s\n", file_path);
            break;
        }

        ret = record_seek_file(pCMP4Read, offset_time);
        if (ret < 0) {
            ERR("tencent player seek record file:%s failed\n", file_path);
            break;
        }

        if (video_type != player->mp4_info.video.pre_type && player->mp4_info.video.pre_type != 0) {
            player->mp4_info.video.video_type_change = TRUE;
        }

        player->mp4_info.serial = -1;
        player->mp4_info.pCMP4Read = pCMP4Read;
        player->mp4_info.video.fps = video_fps;
        player->mp4_info.audio.exist = (REPLAY_SPEED_1X == player->ctrl.speed ? audio_exist : FALSE);
        player->mp4_info.audio.fps = audio_fps;
        player->mp4_info.video.type = (MP4_VIDEO_TYPE_E)video_type;
        player->mp4_info.video.pre_type = (MP4_VIDEO_TYPE_E)video_type;
        player->mp4_info.video.timestamp_ms = (unsigned int)player->rec_info.seek_timestamp_ms;
        player->mp4_info.video.sei_timestamp_ms = (unsigned int)player->rec_info.seek_timestamp_ms;
        player->mp4_info.audio.timestamp_ms = player->mp4_info.video.timestamp_ms;

        DBG("%s player->mp4_info.video.fps=%f\n", __func__, player->mp4_info.video.fps);

        player->ctrl.current_time_ms = mono_msec();
        player->ctrl.start_time_ms = player->ctrl.current_time_ms;
        player->ctrl.duration_v_t = 0;
        player->ctrl.duration_a_t = 0;

        if (VIDEO_H265 == video_type) {
            //player->lv_v_param.format = LV_VIDEO_FORMAT_H265;
            record_get_vps_pps_sps(pCMP4Read, &player->mp4_info.video.vsp_sps_pps_info);
        } else {
            //player->lv_v_param.format = LV_VIDEO_FORMAT_H264;
            record_get_pps_sps(pCMP4Read, &player->mp4_info.video.sps_pps_info);
        }

        return 0;
    }while(0);

    if (pCMP4Read != NULL) {
        pCMP4Read->Close();
        delete pCMP4Read;
        pCMP4Read = NULL;
    }

    return -1;
}

int replay_init_chn(int channel, int record_id, iv_cm_time_fragment_s *pb_time, iv_cm_av_data_info_s *pstAvDataInfo)
{
    int ret = 0;
    RecordPlayChnS* player = NULL;
    char file_path[MAX_REC_FILENAME_LEN] = {0};

    do {
        player = (RecordPlayChnS*)malloc(sizeof(RecordPlayChnS));
        if (NULL == player) {
            ERR("player init malloc failed\n");
            return -1;
        }
        memset(player, 0, sizeof(RecordPlayChnS));

        player->ctrl.speed = REPLAY_SPEED_1X;
        player->ctrl.play_status= STORAGE_RECORD_START;//STORAGE_RECORD_STOP;
        player->mng.id = record_id;

        player->rec_info.start_time_s = pb_time->begin_time_s;
        player->rec_info.stop_time_s = pb_time->end_time_s;
        player->rec_info.seek_timestamp_ms = pb_time->type;
        player->rec_info.seek_time_s = 0;//param->by_utc.seek_time;
        dbg_tencent("start_time_s = %u, stop_time_s = %u\n", player->rec_info.start_time_s, player->rec_info.stop_time_s);
        ret = replay_find_record_list(&player->rec_info, channel);
        if (ret < 0) {
            ERR("tencent player fail to find record list\n");
            break;
        }

        ret = replay_find_record_file(&player->rec_info, file_path, sizeof(file_path));
        if (ret < 0) {
            ERR("tencent player fail to find record file\n");
            break;
        }

        ret = replay_open_record_file(player, file_path);
        if (ret < 0) {
            ERR("tencent player fail to open record file:%s\n", file_path);
            break;
        }


        pstAvDataInfo->u32Framerate = player->mp4_info.video.fps;
        pstAvDataInfo->eVideoType =   (VIDEO_H265 == player->mp4_info.video.type)?IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;

        int width = 0;
        int height = 0;

        VideoEncS videoenc = {0};
        RecordCtrlS record_ctrl = {0};

        conf_get_videocfg(&videoenc);
        conf_get_recordcfg(&record_ctrl);

        if (MAX_SENSOR_NUM > 1) {
            if (record_ctrl.rec_type == VIDEO_CHN_MAIN) { //主码流
                encode_vencsize_to_resolution(videoenc.enc[channel].vencsize, &width, &height);
            } else {
                encode_vencsize_to_resolution(videoenc.enc[2].vencsize, &width, &height);
            }
        } else {
            encode_vencsize_to_resolution(videoenc.enc[record_ctrl.rec_type].vencsize, &width, &height);
        }
        pstAvDataInfo->u32VideoWidth = width;
        pstAvDataInfo->u32VideoHeight = height;

        // audio
        pstAvDataInfo->eAudioType = IV_CM_AENC_TYPE_AAC;
        pstAvDataInfo->eAudioMode = IV_CM_AENC_MODE_MONO;//单声道
        pstAvDataInfo->eAudioBitWidth = IV_CM_AENC_BIT_WIDTH_16;
        pstAvDataInfo->eAudioSampleRate = IV_CM_AENC_SAMPLE_RATE_16000;
        pstAvDataInfo->u32SampleNumPerFrame = AAC_SMPL_PER_FRM_16K;
        replay_init = 1;
        g_replay.push_back(player);
        WriteFile("/proc/sys/vm/drop_caches", "3");
        return 0;
    }while(0);

    if (player->rec_info.file_list != NULL) {
        free(player->rec_info.file_list);
        player->rec_info.file_list = NULL;
    }

    if (NULL != player) {
        free(player);
        player = NULL;
    }

    return -1;
}

int replay_uninit_chn(int id)
{
    unsigned int i = 0;
    RecordPlayChnS* player = NULL;

    for(i = 0; i < g_replay.size(); i++) {
        if (id == g_replay[i]->mng.id) {
            player = g_replay[i];
            break;
        }
    }

    if (NULL != player) {
        g_replay.erase(g_replay.begin()+i);

        if (NULL != player->mp4_info.pCMP4Read) {
            player->mp4_info.pCMP4Read->Close();
            delete player->mp4_info.pCMP4Read;
            player->mp4_info.pCMP4Read = NULL;
        }

        if (player->rec_info.file_list != NULL) {
            free(player->rec_info.file_list);
            player->rec_info.file_list = NULL;
        }

        free(player);
        player = NULL;

        DBG("tencent replay uninit chn:%d success, id:%d\n", i, id);
    } else {
        ERR("tencent replay uninit chn not exist, id:%d\n", id);
        return -1;
    }

    return 0;
}

void replay_stop_allchn()
{
    if (g_replay.empty()) {
        return;
    }

    unsigned int i = 0;
    RecordPlayChnS* player = NULL;

    for (i = 0; i < g_replay.size(); i++) {
        player = g_replay[i];
        js_delete_timer_r(&player->mng.handle);

        if (player->mp4_info.pCMP4Read != NULL) {
            player->mp4_info.pCMP4Read->Close();
            player->mp4_info.pCMP4Read = NULL;
        }

        if (player->rec_info.file_list != NULL) {
            free(player->rec_info.file_list);
            player->rec_info.file_list = NULL;
        }

        free(player);
        player = NULL;
    }

    g_replay.clear();
}

void* replay_get_chn(int id)
{
    unsigned int i = 0;
    for (i = 0; i < g_replay.size(); i++) {
        if (id == g_replay[i]->mng.id) {
            return (void*)g_replay[i];
        }
    }

	return NULL;
}

static int replay_reopen_record_file(RecordPlayChnS* player)
{
    char file_path[128] = {0};
    int ret = 0;
    RecordInfoS* rec_info = &player->rec_info;
    Mp4InfoS* mp4_info = &player->mp4_info;
    char *pData = NULL;
    char *video_buf = NULL;

    do {
        video_buf = (char *)calloc(1, FRAME_HEAD_PRE_SIZE + REC_VIDEO_BUF_SIZE);
        if (NULL == video_buf) {
            SYSLOG("calloc video_buf failed!\n");
            break;
        }

        pData = video_buf + FRAME_HEAD_PRE_SIZE;

        snprintf(file_path, sizeof(file_path) - 1, "%s%s", rec_info->file_dir,
                 rec_info->file_list[rec_info->play_num].file_name);
        rec_info->seek_timestamp_ms = mp4_info->video.timestamp_ms;

        ret = replay_open_record_file(player, file_path);
        if (0 < ret) {
            WAR("re-open failed!!!\n");
            break;
        }

        if (mp4_info->pCMP4Read == NULL) {
            WAR("pCMP4Read is NULL\n");
            break;
        }

        int video_frame_size = 0, frame_type = 0;
        unsigned int duration_v = 0;
        int frame_count = record_read_vframe(mp4_info->pCMP4Read, &mp4_info->serial, pData, REC_VIDEO_BUF_SIZE,
                                                &video_frame_size, &frame_type, &duration_v,
                                                mp4_info->video.type);
        if (frame_count <= 0) {
            WAR("I can not get record stream!!!\n");
            break;
        } else {
            DBG("I can get record stream!!!\n");
            mp4_info->pCMP4Read->SeekToFrame(mp4_info->pCMP4Read->GetCurVideoFrame());

            if (NULL != video_buf) {
                free(video_buf);
            }
            return 0;
        }
    } while (0);

    if (NULL != player->mp4_info.pCMP4Read) {
        player->mp4_info.pCMP4Read->Close();
        delete player->mp4_info.pCMP4Read;
        player->mp4_info.pCMP4Read = NULL;
    }

    if (NULL != video_buf) {
        free(video_buf);
    }

    return -1;
}

int play_next_record_file(RecordPlayChnS* player)
{
    int ret = 0;
    RecordInfoS* rec_info = &player->rec_info;

    if (rec_info == NULL) {
        ERR("play next record file rec_info is NULL\n");
        return -1;
    }

    do {
        do {
            // 不是列表最后一个文件，播放下一个文件
            if (rec_info->play_num < rec_info->file_count - 1) {
                rec_info->play_num++;
                break;
            }

            /* 已经是列表最后一个文件，判断是否是正在录像的文件
             * 不是正在录像的文件，没有文件可以继续播放，返回
             * 是正在录像的文件，重新打开该文件可以继续播放
             * 播放失败，可能是文件已经重命名，尝试重新加载列表，再次播放该文件
             */
            if (rec_info->file_list[rec_info->play_num].is_tmp_file != TRUE) {
                return -1;
            }

            ret = replay_reopen_record_file(player);
            if (ret < 0) {
                if (player->rec_info.file_list != NULL) {
                    free(player->rec_info.file_list);
                    player->rec_info.file_list = NULL;
                }
                replay_find_record_list(&player->rec_info, player->rec_info.sensor_id);
                ret = replay_reopen_record_file(player);
            }

            return ret;
        } while (0);

        char file_path[128] = {0};
        snprintf(file_path, sizeof(file_path) - 1, "%s%s", rec_info->file_dir, 
                rec_info->file_list[rec_info->play_num].file_name);
        rec_info->seek_timestamp_ms = (rec_info->file_list[rec_info->play_num].start_time -
                rec_info->start_time_s)*1000;
        
        WAR("total:%d file_no:%d name:%s day_start_time:%u, file_start_time:%u,"
            "file_end_time:%u, seek_time_stamp:%u, res_seek:%u, filesize:%d, server_id:%d\n", 
            rec_info->file_count, rec_info->play_num,
            rec_info->file_list[rec_info->play_num].file_name, rec_info->start_time_s,
            rec_info->file_list[rec_info->play_num].start_time,
            rec_info->file_list[rec_info->play_num].stop_time,
            rec_info->seek_timestamp_ms/1000,
            rec_info->seek_time_s, rec_info->file_list[rec_info->play_num].file_secs,
            player->mng.id);

        ret = replay_open_record_file(player, file_path);
        if (0 == ret) {
            break;
        }
    } while(1);
        
    return 0;
}

void replay_push_record(void *userdata)
{
    int ret = 0;
    static int video_type = 0;
    int video_size = 0;
    unsigned int sei_timestamp = 0;
    BOOL is_key_frame = FALSE;
    char audio_buf[REC_AUDIO_BUF_SIZE] = {0};
	unsigned int duration_v = 0;

    RecordPlayChnS* player = (RecordPlayChnS*) userdata;
    char *pData = NULL;
    char *video_buf = (char *)calloc(1, FRAME_HEAD_PRE_SIZE + REC_VIDEO_BUF_SIZE);
    if (NULL == video_buf) {
        SYSLOG("calloc video_buf failed!\n");
        return;
    }

    pData = video_buf + FRAME_HEAD_PRE_SIZE;

    player->ctrl.current_time_ms = mono_msec();

    iv_cm_venc_stream_s v_stream   = {0};
    iv_cm_venc_pack_s venc_packet = {0};

    if (player->mp4_info.video.video_type_change == TRUE) {
        DBG("video type is change\n");
        char msg_buf[128]          = {0};
        unsigned char rcv_buf[128] = {0};
        size_t recv_len            = sizeof(rcv_buf) - 1;
        snprintf(msg_buf, sizeof(msg_buf),
                "{\"sensor_id\":%d, \"video_type\":%d, \"timestamp\":%u}",
                player->rec_info.sensor_id, video_type, 
                player->rec_info.start_time_s + player->rec_info.seek_timestamp_ms/1000);
        iv_avt_send_command(player->rec_info.visitor, msg_buf, strlen(msg_buf), rcv_buf, &recv_len, 1000);
        if (recv_len > 0) {
            //此处加上截止符是为了让打印输出完整
            rcv_buf[recv_len] = '\0';
            DBG("recv len %d data %s\n", recv_len, rcv_buf);
        }
        player->mp4_info.video.video_type_change = FALSE;
        js_delete_timer_r(&player->mng.handle);
        goto __exit;
    }
    video_type = player->mp4_info.video.type;
    if (STORAGE_RECORD_PAUSE == player->ctrl.play_status || STORAGE_RECORD_STOP == player->ctrl.play_status) {
        player->ctrl.start_time_ms = player->ctrl.current_time_ms;
        player->ctrl.duration_v_t = player->ctrl.duration_a_t = 0;
    } else if (STORAGE_RECORD_START == player->ctrl.play_status || STORAGE_RECORD_UNPAUSE == player->ctrl.play_status) {
        unsigned int send_interval = player->ctrl.current_time_ms - player->ctrl.start_time_ms;

        if (send_interval > player->ctrl.duration_v_t) {
            if (TRUE == player->ctrl.key_only) {
                player->mp4_info.serial = -1;
            }
            /*if (player->run.idrfrm_send_cnt > 0 && TRUE == player->run.mp4_info_sync_done) {
                DBG("send frame head %d\n", player->run.idrfrm_send_cnt);
                ret = record_seek_file(player->mp4_info.pCMP4Read, 0);
                if (ret < 0) {
                    ERR("tencent player seek record file failed\n");
                    goto __exit;
                }

                player->mp4_info.serial = -1;
                player->mp4_info.video.timestamp_ms = (unsigned int)player->rec_info.seek_timestamp_ms;
                player->mp4_info.video.sei_timestamp_ms = (unsigned int)player->rec_info.seek_timestamp_ms;
                player->mp4_info.audio.timestamp_ms = player->mp4_info.video.timestamp_ms;
                player->ctrl.current_time_ms = mono_msec();
                player->ctrl.start_time_ms = player->ctrl.current_time_ms;
                player->ctrl.duration_v_t = 0;
                player->ctrl.duration_a_t = 0;
            }*/
            player->run.frame_count = record_read_vframe(player->mp4_info.pCMP4Read,
                    &player->mp4_info.serial, pData, REC_VIDEO_BUF_SIZE+FRAME_HEAD_PRE_SIZE,
                    &player->run.video_frame_size, &player->run.frame_type,
                    &duration_v, video_type);
            if (player->run.frame_count < 0) {
                ERR("record_read_vframe failed\n");
                player->run.video_timestamp_ms = player->run.audio_timestamp_ms;
				if (NULL != player->mp4_info.pCMP4Read) {
					player->mp4_info.pCMP4Read->Close();
                    delete player->mp4_info.pCMP4Read;
					player->mp4_info.pCMP4Read = NULL;
				}

                ret = play_next_record_file(player);
                if (ret < 0) {  //播放完当天最后一个文件
                    player->ctrl.play_status = STORAGE_RECORD_STOP;
                    DBG("played the last record file\n");
                    iv_avt_send_finish_stream(player->rec_info.visitor, player->rec_info.sensor_id, IV_AVT_VIDEO_RES_PB);
                }
                //player->run.idrfrm_send_cnt = IDR_FRMAE_SEND_TIME;
                goto __exit;
            } /*else {
                if (0 == player->run.idrfrm_send_cnt) {
                    player->run.idrfrm_send_cnt = 1;
                }
            }

            player->run.idrfrm_send_cnt--;
            if (0 == player->run.idrfrm_send_cnt) {
                player->run.mp4_info_sync_done = FALSE;
            }*/
            player->mp4_info.video.sei_timestamp_ms += (int)((player->run.frame_count*1.0*1000/player->mp4_info.video.fps));

            if (SHM_FRAME_VIDEO_I == player->run.frame_type) {
                sei_timestamp = player->rec_info.start_time_s + player->mp4_info.video.sei_timestamp_ms/1000.0;
                // 将时间格式化为字符串
                char buffer[16] = {0,};
                sprintf(buffer, "%u", sei_timestamp);
                player->mp4_info.video.sei_size = SEI_MAX_LEN;
                is_key_frame = TRUE;
                memset(player->mp4_info.video.sei_buffer, 0, sizeof(player->mp4_info.video.sei_buffer));
                if (VIDEO_H265 == video_type) {
                    sVpsSpsPpsInfo* info = &player->mp4_info.video.vsp_sps_pps_info;
                    video_size = info->vps_size + info->sps_size + info->pps_size +
                        player->run.video_frame_size + player->mp4_info.video.sei_size;
                    pData -= (info->vps_size + info->sps_size + info->pps_size + 
                        player->mp4_info.video.sei_size);
                    memmove(pData, info->vps_buf, info->vps_size);
                    memmove(pData + info->vps_size, info->sps_buf, info->sps_size);
                    memmove(pData + info->vps_size + info->sps_size, info->pps_buf, info->pps_size);

                    construct_sei(video_type, player->mp4_info.video.sei_buffer,
                            player->mp4_info.video.sei_size, buffer);
                    memmove(pData + info->vps_size + info->sps_size + info->pps_size,
                            player->mp4_info.video.sei_buffer,
                            player->mp4_info.video.sei_size);
                } else {
                    
                    sSpsPpsInfo* info = &player->mp4_info.video.sps_pps_info;
                    video_size = info->sps_size + info->pps_size +
                            player->run.video_frame_size + player->mp4_info.video.sei_size;
                    pData -= (info->sps_size + info->pps_size +
                            player->mp4_info.video.sei_size);
                    memmove(pData, info->sps_buf, info->sps_size);
                    memmove(pData + info->sps_size, info->pps_buf, info->pps_size);
                    construct_sei(video_type, player->mp4_info.video.sei_buffer,
                            player->mp4_info.video.sei_size, buffer);
                    memmove(pData + info->sps_size + info->pps_size,
                            player->mp4_info.video.sei_buffer,
                            player->mp4_info.video.sei_size);
                }
            } else {
                is_key_frame = FALSE;
                video_size = player->run.video_frame_size;
            }

            if (is_key_frame || !player->ctrl.key_only) {
                venc_packet.pu8Addr = (uint8_t *)pData;
                venc_packet.u32Len  = (uint32_t)video_size;
                venc_packet.u64PTS  = player->run.video_timestamp_ms;
                venc_packet.u32Seq  = player->mp4_info.serial;
                venc_packet.eFrameType = (TRUE == is_key_frame) ? IV_CM_FRAME_TYPE_I : IV_CM_FRAME_TYPE_P;

                v_stream.u32PackCount = 1;
                v_stream.pstVencPack[0] = &venc_packet;
                dbg_tencent("VIDEO: v_timestamp_ms=%u, frame_type=%d, serial=%d\n",
                        player->mp4_info.video.timestamp_ms, player->run.frame_type,
                        player->mp4_info.serial);
                ret = iv_avt_send_stream(player->rec_info.visitor, player->rec_info.sensor_id,
                        IV_AVT_VIDEO_RES_PB, IV_AVT_STREAM_TYPE_VIDEO, &v_stream);
                if (ret < 0) {
                    ERR("record send video stream fail:%d record_id:%d, video_type = %d\n",
                            ret, player->mng.id, video_type);
                    player->ctrl.play_status = STORAGE_RECORD_STOP;
                    iv_avt_send_finish_stream(player->rec_info.visitor,
                            player->rec_info.sensor_id, IV_AVT_VIDEO_RES_PB);
                    goto __exit;
                }
            }

            player->mp4_info.video.timestamp_ms += (int)(duration_v * 1000.0 / 90000)/player->ctrl.speed;
            player->ctrl.duration_v_t += (int)((duration_v * 1000.0 / 90000)/player->ctrl.speed);
            player->run.video_timestamp_ms += (int)((duration_v * 1000 / 90000) / player->ctrl.speed);
            player->mp4_info.serial++;
            dbg_tencent("VIDEO: v_timestamp_ms=%d, frame_type=%d, serial=%d\n",
                    player->run.video_timestamp_ms, player->run.frame_type,
                    player->mp4_info.serial);
        }

        if ((player->mp4_info.audio.exist) && (send_interval >= player->ctrl.duration_a_t)) {
            ret = record_read_aframe(player->mp4_info.pCMP4Read, audio_buf, sizeof(audio_buf),
                    &player->run.audio_frame_size, &player->run.duration_a);
            if (ret < 0) {
                DBG("read audio over\n");
                player->mp4_info.audio.exist = FALSE;
                goto __exit;
            }

            iv_cm_aenc_stream_s a_stream   = {0};
            iv_cm_aenc_pack_s aenc_packet = {0};


            if (player->run.audio_frame_size > 0) {
                aenc_packet.pu8Addr = (uint8_t *)audio_buf;
                aenc_packet.u32Len  = (uint32_t)player->run.audio_frame_size;
                aenc_packet.u64PTS  = player->run.audio_timestamp_ms;
                aenc_packet.u32Seq  = player->mp4_info.serial;

                a_stream.u32PackCount   = 1;
                a_stream.pstAencPack[0] = &aenc_packet;
                ret = iv_avt_send_stream(player->rec_info.visitor, player->rec_info.sensor_id,
                        IV_AVT_VIDEO_RES_PB, IV_AVT_STREAM_TYPE_AUDIO, &a_stream);
                if (ret < 0) {
                    ERR("record send audio stream fail:%d record_id:%d\n", ret, player->mng.id);
                    player->ctrl.play_status = STORAGE_RECORD_STOP;
                    iv_avt_send_finish_stream(player->rec_info.visitor, player->rec_info.sensor_id, IV_AVT_VIDEO_RES_PB);
                }
            }

            player->run.audio_timestamp_ms += 1.0 * AAC_FRM_DURATION_16K/player->ctrl.speed;
            player->mp4_info.audio.timestamp_ms += 1.0 * AAC_FRM_DURATION_16K/player->ctrl.speed;
            player->ctrl.duration_a_t += 1.0 * AAC_FRM_DURATION_16K/player->ctrl.speed;
            dbg_tencent("AUDIO: a_timestamp_ms=%d, audio_aac_length=%d\n",
                    player->run.audio_timestamp_ms, player->run.audio_frame_size);
        }

        /* 回放过程中音视频时间戳相差过大，会导致APP上回放跳帧, 需同步下音视频的时间戳 */
        if (player->ctrl.speed != REPLAY_SPEED_4X && player->mp4_info.audio.exist == TRUE) {
            int duration_inter = player->ctrl.duration_a_t-player->ctrl.duration_v_t;
            if (abs(duration_inter) > 400) {
                if (duration_inter > 400) {//音频快
                    player->mp4_info.video.timestamp_ms = player->mp4_info.audio.timestamp_ms;
                    player->run.video_timestamp_ms = player->run.audio_timestamp_ms;
                } else {//视频快
                    player->mp4_info.audio.timestamp_ms = player->mp4_info.video.timestamp_ms;
                    player->run.audio_timestamp_ms = player->run.video_timestamp_ms;
                }

                player->ctrl.start_time_ms = player->ctrl.current_time_ms;
                player->ctrl.duration_v_t = player->ctrl.duration_a_t = 0;
                DBG("fix the record time stamp inter:%d\n", duration_inter);
            }
        }
    }

__exit:
    if (NULL != video_buf) {
        free(video_buf);
    }
    return;
}

void replay_seek_record_file(RecordPlayChnS* player)
{
    int ret = 0;
    char file_path[MAX_REC_FILENAME_LEN] = {0};

    if (player->mp4_info.pCMP4Read) {
        player->mp4_info.pCMP4Read->Close();
        delete(player->mp4_info.pCMP4Read);
        player->mp4_info.pCMP4Read = NULL;
    }

    /* 如果切换回放日期，云端会重新下发service_id，所以不需要重新查询录像 */
    ret = replay_find_record_file(&player->rec_info, file_path, sizeof(file_path));
    if (ret < 0 || (!is_okey(file_path))) {
        WAR("not find record file,refresh list refind\n");

        if (player->rec_info.file_list != NULL) {
            free(player->rec_info.file_list);
            player->rec_info.file_list = NULL;
        }

        ret = replay_find_record_list(&player->rec_info, player->rec_info.sensor_id);
        if (ret < 0) {
            ERR("player fail to find record list\n");
            return;
        } else {
            ret = replay_find_record_file(&player->rec_info, file_path,
                                          sizeof(file_path));
            if (ret < 0) {
                ERR("fail to seek_00am_off_secs:%u\n", player->rec_info.seek_timestamp_ms/1000);
                return;
            }
        }
    }

    ret = replay_open_record_file(player, file_path);
    if (ret < 0) {
        ERR("tencent player fail to open record file\n");
        return;
    }

    player->ctrl.play_status = STORAGE_RECORD_START;
}

int get_record_process(int record_id, iv_avt_video_res_type_e video_res_type)
{
    if(!replay_init){
        return 0;
    }

    int process = 0;
    int index = replay_get_paly_chn(record_id);
    RecordPlayChnS* player = g_replay[index];

    //必须在回放推流速度与小程序播放录像速度保持一致，才能保证获取的进度是相对准确的
    //云端缓存录像数据的cache只能保存0.8秒，可以适当减500~800ms，来保证进度准确
    if (player->mng.time == 50 && player->mp4_info.video.timestamp_ms > 1000) {
        process = player->mp4_info.video.timestamp_ms - 500;
    } else {
        process = player->mp4_info.video.timestamp_ms;
    }

    //每次seek后，因为推送的第1帧是I帧，而timestamp_ms确实seek下发的时间戳，这就会无法避免地导致有个3秒内的误差

    dbg_record("***********process=%d\n", process);
    return process;

}

void replay_push_cmd(void * usrdata)
{
    RecordplayCmdS* cmd = (RecordplayCmdS*)usrdata;
    if (NULL == cmd) {
        ERR("tencent player push cmd, ctrl is NULL\n");
        return;
    }

    DBG("on_push_stream_cmd_cb ctrl->play_status = %d\n", cmd->play_status);
    do {
        int index = replay_get_paly_chn(cmd->id);
        if (index < 0) {
            DBG("index=%d\n", index);
            break;
        }
        RecordPlayChnS* player = g_replay[index];
        if (NULL == player) {
            ERR("RECORD is already stop!\n");
            break;
        }

        switch(cmd->play_status) {
            case STORAGE_RECORD_START:
            case STORAGE_RECORD_UNPAUSE:
            case STORAGE_RECORD_PAUSE:
            case STORAGE_RECORD_STOP:
                player->ctrl.play_status = cmd->play_status;
                break;

            case STORAGE_RECORD_SET_PARAM: {
                if (player->ctrl.key_only != cmd->key_only) {
                    player->ctrl.key_only = cmd->key_only;
                    DBG("LV_STORAGE_RECORD_SET_PARAM key_only:%d\n", cmd->key_only);
                }

                if ((cmd->speed == REPLAY_SPEED_1X || cmd->speed == REPLAY_SPEED_2X 
                    || cmd->speed == REPLAY_SPEED_4X) && player->ctrl.speed != cmd->speed) {
                    player->ctrl.speed = cmd->speed;
                    int new_time = (cmd->speed == REPLAY_SPEED_2X)?20:STREAM_PUSH_TIMER;
                    if (cmd->speed != REPLAY_SPEED_1X) {
                        player->mp4_info.audio.exist = FALSE;
                    } else {
                        player->mp4_info.audio.exist = TRUE;
                    }
                    js_modify_timer_time_r(&player->mng.handle, new_time);
                    DBG("LV_STORAGE_RECORD_SET_PARAM speed:%d, play timer:%d\n", cmd->speed, player->mng.time);
                }
                break;
            }

            case STORAGE_RECORD_SEEK: {  //seek 暂停推录像
                DBG("seek_timestamp_ms=%d\n", cmd->timestamp_ms);
                player->rec_info.seek_timestamp_ms = cmd->timestamp_ms;
                player->ctrl.play_status = STORAGE_RECORD_PAUSE;
                player->run.video_timestamp_ms = player->run.audio_timestamp_ms;
                replay_seek_record_file(player);
                break;
            }

            default:
                break;
        }
    } while(0);

    free(cmd);
    cmd = NULL;
}

#endif //PLATFORM_TENCENT
