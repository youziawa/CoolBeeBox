#ifndef _TEMP_CONTROL_H_
#define _TEMP_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

// 初始化温控模块
void temp_control_init(void);

// 更新当前温度 (由 BME680 任务调用)
void temp_control_update_temperature(float temperature);

#ifdef __cplusplus
}
#endif

#endif // _TEMP_CONTROL_H_
