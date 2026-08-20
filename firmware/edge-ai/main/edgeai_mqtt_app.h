#ifndef _EDGEAI_MQTT_APP_H_
#define _EDGEAI_MQTT_APP_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void edgeai_mqtt_app_start(void);
void edgeai_mqtt_publish_state(const char *label, float confidence, bool stress);

#ifdef __cplusplus
}
#endif

#endif // _EDGEAI_MQTT_APP_H_
