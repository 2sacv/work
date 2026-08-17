#ifndef _JEVENT_H
#define _JEVENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "js_event.h"
#include "jconfig.h"

JSEventManager *get_ev_mng_alarm(void);

void send_event(JSEventType id);
int init_server_event();
void send_event_chn(JSEventType id, int chn);
void send_event_data(JSEventType id, void *cb_data);

#define attach_event(id, cb_func, cb_data) do { \
    if (0 != js_event_attach(get_ev_mng_alarm(), id, cb_func, cb_data)) { \
            ERR("attach failed in %s\n", __func__); \
        } \
    } while(0)

#define attach_event_async(id, cb_func, cb_data) do { \
    if (0 != js_event_attach_async(get_ev_mng_alarm(), id, cb_func, cb_data)) { \
            ERR("attach failed in %s\n", __func__); \
        } \
    } while(0)

#define detach_event(id, cb_func, cb_data) do { \
    if (0 != js_event_detach(get_ev_mng_alarm(), id, cb_func, cb_data)) { \
        ERR("detach failed in %s\n", __func__); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
#endif
