//
// Created by monktan on 2024/3/11.
//

#ifndef IOT_VIDEO_CLIENT_IV_CLI_V2_H
#define IOT_VIDEO_CLIENT_IV_CLI_V2_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "iv_av.h"
#include "iv_av_cli_utils.h"
#include <stdbool.h>

struct iv_client_func_t {
    /// log print callback
    /// @param[in] msg log message
    void (*log_print)(const char *msg);

    /// start stream param callback
    /// @param[in] conn_id connection id
    /// @param[in] stream_type stream type
    /// @param[in] p_av_data_info audio/video data info
    /// @param[in] param user defined param
    void (*on_start_stream)(const char *conn_id, iv_avt_stream_type_e stream_type, iv_cm_av_data_info_s *p_av_data_info, void *param);

    /// stop stream param callback
    /// @param[in] conn_id connection id
    /// @param[in] stream_type stream type
    void (*on_stop_stream)(const char *conn_id, iv_avt_stream_type_e stream_type, void *param);

    /// receive stream packet callback
    /// @param[in] conn_id connection id
    /// @param[in] stream_type stream type
    /// @param[in] p_stream audio/video stream data
    /// @param[in] param user defined param
    void (*on_stream_packet)(const char *conn_id, iv_avt_stream_type_e stream_type, void *p_stream, void *param);

    /// event notify callback
    /// @param[in] conn_id connection id
    /// @param[in] event event type
    /// @param[in] param user defined param
    void (*on_event_notify)(const char *conn_id, iv_avt_event_e event, void *param);

    /// peer feedback callback
    /// @param[in] conn_id connection id
    /// @param[in] msg message data
    /// @param[in] msg_len message length
    /// @param[out] resp response data
    /// @param[out] resp_len response length
    /// @param[out] free free function
    void (*on_peer_feedback)(const char *conn_id, const unsigned char *msg, size_t msg_len, uint8_t **resp, int32_t *resp_len, void (**free)(void *p), void *param);
};

// peer info
struct iv_peer_info {
    char *product_id;
    char *device_name;
    char *xp2p_info;
};

// stream info
struct iv_stream_info {
    /* channel: always 0 for now */
    uint32_t channel;
    /* disable stream encryption/decryption during transfer */
    bool disable_crypt;
    /* video resolution/quality: standard/high/super, for recv stream */
    iv_avt_video_res_type_e res_type;
    /*max size of stream frame (kB), for send stream*/
    size_t max_frame_size;
};

/// create client
/// @param[in] func callback function
/// @param[in] param user defined param
/// @return client handle, NULL if failed
struct iv_client_t;
struct iv_client_t* iv_client_create(const struct iv_client_func_t *func, void* param);

/// destroy client
/// @param[in] client client handle
/// @return 0-success, other-failed
int iv_client_destroy(struct iv_client_t* client);

/// setup connect
/// @param[in] client client handle
/// @param[in] peer_info peer info
/// @return 0-success, other-failed
int iv_client_setup_connect(struct iv_client_t* client, const struct iv_peer_info *peer_info);

/// teardown connect
/// @param[in] client client handle
/// @return 0-success, other-failed
int iv_client_teardown_connect(struct iv_client_t* client);

/// start recv stream
/// @param[in] client client handle
/// @param[in] info stream info
/// @param[in] rid resource id
/// @param[in] requester requester
/// @return 0-success, other-failed
int iv_client_start_recv_stream(struct iv_client_t* client,
                                struct iv_stream_info *info,
                                const char *rid,
                                const char *requester);

/// stop recv stream
/// @param[in] client client handle
/// @return 0-success, other-failed
int iv_client_stop_recv_stream(struct iv_client_t* client);


/// start send stream
/// @param[in] client client handle
/// @param[in] info stream info
/// @param[in] av_data_info audio/video data info
/// @return 0-success, other-failed
int iv_client_start_send_stream(struct iv_client_t* client,
                                struct iv_stream_info *info,
                                iv_cm_av_data_info_s *av_data_info);

/// stop send stream
/// @param[in] client client handle
/// @return 0-success, other-failed
int iv_client_stop_send_stream(struct iv_client_t* client);

/// send stream
/// @param[in] client client handle
/// @param[in] stream_type stream type
/// @param[in] p_stream stream data, real data type is iv_cm_venc_stream_s or iv_cm_aenc_pack_s
/// @return 0-success, other-failed
int iv_client_send_stream(struct iv_client_t* client, iv_avt_stream_type_e stream_type, void *p_stream);

/// send command
/// @param[in] client client handle
/// @param[in] command command data
/// @param[in] command_len command data length
/// @param[out] recv_buf response data
/// @param[out] recv_len response data length
/// @param[in] timeout_ms timeout in ms
/// @return 0-success, other-failed
int iv_client_send_command(struct iv_client_t* client,
                           const uint8_t* command,
                           size_t command_len,
                           uint8_t**recv_buf,
                           size_t *recv_len,
                           uint32_t timeout_ms);

/// get recv buffer size
/// @param[in] client client handle
/// @return buffer size, 0 if failed
size_t iv_client_get_send_buffer_size(struct iv_client_t* client);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // IOT_VIDEO_CLIENT_IV_CLI_V2_H
