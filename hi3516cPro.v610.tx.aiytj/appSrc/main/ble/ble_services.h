#ifndef __BLE_SERVICES_H__
#define __BLE_SERVICES_H__
#ifdef __cplusplus
extern "C" {
#endif

int ble_services_start(void);
void ble_services_stop(void);
int ble_parse_rx_data(char *recv_buf, uint16_t att_hdl);

#ifdef __cplusplus
}
#endif
#endif
