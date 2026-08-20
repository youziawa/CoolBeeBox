#ifndef _WIFI_APP_H_
#define _WIFI_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MY_WIFI_SSID
#define MY_WIFI_SSID      "YOUR_WIFI_SSID"
#endif
#ifndef MY_WIFI_PASS
#define MY_WIFI_PASS      "YOUR_WIFI_PASSWORD"
#endif
#define MY_MAXIMUM_RETRY  5

void wifi_init_sta(void);

#ifdef __cplusplus
}
#endif

#endif // _WIFI_APP_H_
