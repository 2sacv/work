#ifndef _AGING_TEST_H
#define _AGING_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#define AGING_TEST_KILL     "killall aging_test.sh"
#define AGING_TEST_MNT      "/mnt/aging_test.sh"
#define AGING_TEST_OPT      "/opt/aging_test.sh"
#define AGING_TEST_KEEP     "/opt/keep_aging_test"

enum CHN{
    led_white_open = 2,
    led_white_close = 3,
    led_infrared_open = 4,
    led_infrared_close = 5,
    led_ircut_redglass_open = 6,
    led_ircut_whiteglass_open = 7,
};

enum AGTEST_TYPE{
    AGTEST_ENABLE = 0,
    AGTEST_IRCUT,
    AGTEST_AUDIO,
    AGTEST_LED_RED,
    AGTEST_LED_WHITE,
    AGTEST_PTZ,
    AGTEST_PTZ_SPEED,
    AGTEST_ZOOM,
    AGTEST_KEEP,
};

int aging_test_process(int type, int value);

void init_aging_test(void);

void uninit_aging_test(void);

#ifdef __cplusplus
}
#endif
#endif
