# CO2传感器集成配置指南

## 1. MySQL数据库修改

### 1.1 添加CO2字段到现有表

```sql
-- 连接到MySQL数据库
mysql -u root -p CoolBeeBox

-- 添加co2字段
ALTER TABLE bee_sensor_data ADD COLUMN co2 DECIMAL(10,2) DEFAULT NULL AFTER gas;

-- 查看表结构确认
DESCRIBE bee_sensor_data;
```

### 1.2 验证字段已添加

```sql
SHOW COLUMNS FROM bee_sensor_data;
```

应该看到：
- id
- device_id
- temperature
- humidity
- pressure
- gas
- co2  ← 新增
- uptime
- recorded_at
- created_at

---

## 2. EMQX规则引擎修改

### 2.1 创建新规则

登录EMQX Dashboard (http://1.94.161.42:18083)

1. 进入 **规则引擎** → **规则**
2. 点击"创建"按钮
3. 配置SQL：

```sql
SELECT 
  payload.temperature AS temperature,
  payload.humidity AS humidity,
  payload.pressure AS pressure,
  payload.gas AS gas,
  payload.co2 AS co2,
  payload.uptime AS uptime,
  'ESP32_001' AS device_id
FROM "device/coolbee/sensors"
```

4. 创建动作：使用以下**正确的SQL模板**：

```sql
INSERT INTO bee_sensor_data (device_id, temperature, humidity, pressure, gas, co2, uptime, recorded_at)
VALUES ('ESP32_001', ${temperature}, ${humidity}, ${pressure}, ${gas}, ${co2}, ${uptime}, NOW())
```

### 2.2 确保数据库表有co2字段

```sql
-- 连接到MySQL
mysql -u root -p CoolBeeBox

-- 检查表结构
SHOW COLUMNS FROM bee_sensor_data;

-- 如果没有co2字段，添加它
ALTER TABLE bee_sensor_data ADD COLUMN co2 DECIMAL(10,2) DEFAULT NULL AFTER gas;
```

---

## 3. 验证配置

### 3.1 测试数据存储

等待ESP32发送新数据后，在MySQL中查询：

```sql
-- 查看最新10条记录
SELECT * FROM bee_sensor_data ORDER BY recorded_at DESC LIMIT 10;

-- 专门查看co2数据
SELECT recorded_at, co2 FROM bee_sensor_data WHERE co2 IS NOT NULL ORDER BY recorded_at DESC LIMIT 10;
```

### 3.2 查看EMQX规则触发情况

在EMQX Dashboard中：
1. 进入 **监控** → **规则引擎**
2. 查看规则的执行统计
3. 检查是否有错误信息

---

## 4. MQTT主题变更说明

### 旧主题
- `device/coolbee/bme680`

### 新主题
- `device/coolbee/sensor`

### 前端已更新
- ✅ mqtt.js 已更新订阅主题
- ✅ dashboard.js 已更新数据解析逻辑
- ✅ DashboardView.vue 已添加CO2显示卡片
- ✅ RealtimeView.vue 已添加CO2显示卡片
- ✅ HistoryView.vue 已添加CO2数据列

---

## 5. CO2浓度参考标准

| CO2浓度 (PPM) | 等级 | 颜色 | 说明 |
|--------------|------|------|------|
| < 800 | 优秀 | 🟢 绿色 | 蜂箱通风良好 |
| 800-1000 | 良好 | 🔵 蓝色 | 正常范围 |
| 1000-1500 | 警告 | 🟠 橙色 | 需要关注通风 |
| > 1500 | 危险 | 🔴 红色 | 立即通风 |

---

## 6. 快速检查清单

- [ ] MySQL数据库已添加co2字段
- [ ] EMQX规则已更新SQL查询
- [ ] EMQX规则已更新动作SQL模板
- [ ] EMQX规则主题已改为 `device/coolbee/sensor`
- [ ] 前端已重新构建并部署
- [ ] 测试数据正常显示
