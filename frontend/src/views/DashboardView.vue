<template>
  <div class="dashboard-container">
    <!-- 顶部状态卡片 -->
    <el-row :gutter="20" class="status-cards">
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><Connection /></el-icon>
              <span>设备连接状态</span>
            </div>
          </template>
          <div class="card-content">
            <div class="status-indicator">
              <el-tag :type="mqttStore.connected ? 'success' : 'danger'" size="large">
                {{ mqttStore.connected ? '已连接' : '未连接' }}
              </el-tag>
              <el-tag v-if="mqttStore.connected" :type="dashboardStore.deviceOnline ? 'success' : 'warning'" size="large" style="margin-left: 10px;">
                {{ dashboardStore.deviceOnline ? '设备在线' : '设备离线' }}
              </el-tag>
              <el-tag v-if="mqttStore.connected" :type="dashboardStore.edgeAiOnline ? 'success' : 'warning'" size="large" style="margin-left: 10px;">
                {{ dashboardStore.edgeAiOnline ? 'EdgeAI 在线' : 'EdgeAI 离线' }}
              </el-tag>
            </div>
            <div class="status-info">
              <p>设备ID: {{ mqttStore.deviceId || '未设置' }}</p>
              <p>Broker: {{ mqttStore.broker || '未配置' }}</p>
              <p v-if="dashboardStore.lastDeviceUptime !== null">
                设备运行: {{ Math.floor(dashboardStore.lastDeviceUptime / 3600) }}h {{ Math.floor((dashboardStore.lastDeviceUptime % 3600) / 60) }}m {{ dashboardStore.lastDeviceUptime % 60 }}s
              </p>
              <p>
                数据来源: 
                <el-tag v-if="dashboardStore.dataSource === 'esp32'" type="success" size="small">ESP32节点1数据</el-tag>
                <el-tag v-else-if="dashboardStore.dataSource === 'simulation'" type="warning" size="small">模拟数据</el-tag>
                <el-tag v-else type="info" size="small">等待数据</el-tag>
              </p>
            </div>
            <div class="connect-button">
              <el-button 
                type="primary" 
                :icon="Refresh" 
                :loading="mqttStore.connecting"
                @click="handleConnectMQTT"
                size="small"
                style="width: 100%; margin-top: 10px;"
              >
                {{ mqttStore.connected ? '重新连接' : '连接EMQX' }}
              </el-button>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card" :class="{ 'alarm-card': temperatureAlarm.status !== 'normal' && temperatureAlarm.status !== 'optimal' }">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><Sunny /></el-icon>
              <span>蜂箱温度</span>
              <el-tag v-if="temperatureAlarm.status !== 'normal' && temperatureAlarm.status !== 'optimal'" :type="temperatureAlarm.level" size="small" style="margin-left: auto;">
                <el-icon v-if="temperatureAlarm.status === 'high'" :color="temperatureAlarm.color"><Warning /></el-icon>
                {{ temperatureAlarm.text }}
              </el-tag>
              <el-tag v-else-if="temperatureAlarm.status === 'optimal'" type="success" size="small" effect="plain" style="margin-left: auto;">
                {{ temperatureAlarm.text }}
              </el-tag>
            </div>
          </template>
          <div class="card-content">
            <div class="metric-value">
              <span class="value" :style="{ color: temperatureAlarm.color }">
                {{ dashboardStore.temperature ? dashboardStore.temperature.toFixed(2) : '--' }}
              </span>
              <span class="unit">°C</span>
            </div>
            <div class="metric-trend">
              <el-icon :color="dashboardStore.temperatureTrend > 0 ? '#f56c6c' : '#67c23a'">
                <ArrowUp v-if="dashboardStore.temperatureTrend > 0" />
                <ArrowDown v-else />
              </el-icon>
              <span :style="{ color: dashboardStore.temperatureTrend > 0 ? '#f56c6c' : '#67c23a' }">
                {{ dashboardStore.temperatureTrend > 0 ? '+' : '' }}{{ dashboardStore.temperatureTrend ? dashboardStore.temperatureTrend.toFixed(2) : 0 }}°C
              </span>
            </div>
            <div class="optimal-range">
              <span>最佳范围: {{ alarmConfig.temperature.optimal }}</span>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card" :class="{ 'alarm-card': humidityAlarm.status !== 'normal' && humidityAlarm.status !== 'optimal' }">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><HotWater /></el-icon>
              <span>蜂箱湿度</span>
              <el-tag v-if="humidityAlarm.status !== 'normal' && humidityAlarm.status !== 'optimal'" :type="humidityAlarm.level" size="small" style="margin-left: auto;">
                <el-icon v-if="humidityAlarm.status === 'high'" :color="humidityAlarm.color"><Warning /></el-icon>
                {{ humidityAlarm.text }}
              </el-tag>
              <el-tag v-else-if="humidityAlarm.status === 'optimal'" type="success" size="small" effect="plain" style="margin-left: auto;">
                {{ humidityAlarm.text }}
              </el-tag>
            </div>
          </template>
          <div class="card-content">
            <div class="metric-value">
              <span class="value" :style="{ color: humidityAlarm.color }">
                {{ dashboardStore.humidity ? dashboardStore.humidity.toFixed(2) : '--' }}
              </span>
              <span class="unit">%</span>
            </div>
            <div class="metric-trend">
              <el-icon :color="dashboardStore.humidityTrend > 0 ? '#f56c6c' : '#67c23a'">
                <ArrowUp v-if="dashboardStore.humidityTrend > 0" />
                <ArrowDown v-else />
              </el-icon>
              <span :style="{ color: dashboardStore.humidityTrend > 0 ? '#f56c6c' : '#67c23a' }">
                {{ dashboardStore.humidityTrend > 0 ? '+' : '' }}{{ dashboardStore.humidityTrend ? dashboardStore.humidityTrend.toFixed(2) : 0 }}%
              </span>
            </div>
            <div class="optimal-range">
              <span>最佳范围: {{ alarmConfig.humidity.optimal }}</span>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card" :class="{ 'alarm-card': pressureAlarm.status !== 'normal' }">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><TrendCharts /></el-icon>
              <span>环境气压</span>
              <el-tag v-if="pressureAlarm.status !== 'normal'" :type="pressureAlarm.level" size="small" style="margin-left: auto;">
                <el-icon v-if="pressureAlarm.status === 'high'" :color="pressureAlarm.color"><Warning /></el-icon>
                {{ pressureAlarm.text }}
              </el-tag>
            </div>
          </template>
          <div class="card-content">
            <div class="metric-value">
              <span class="value" :style="{ color: pressureAlarm.color }">
                {{ dashboardStore.pressure ? dashboardStore.pressure.toFixed(2) : '--' }}
              </span>
              <span class="unit">hPa</span>
            </div>
            <div class="metric-trend">
              <el-icon :color="dashboardStore.pressureTrend > 0 ? '#f56c6c' : '#67c23a'">
                <ArrowUp v-if="dashboardStore.pressureTrend > 0" />
                <ArrowDown v-else />
              </el-icon>
              <span :style="{ color: dashboardStore.pressureTrend > 0 ? '#f56c6c' : '#67c23a' }">
                {{ dashboardStore.pressureTrend > 0 ? '+' : '' }}{{ dashboardStore.pressureTrend ? dashboardStore.pressureTrend.toFixed(2) : 0 }}hPa
              </span>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card stress-card" :class="{ 'alarm-card': beeStressAlarm.status === 'stress' }">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><Warning /></el-icon>
              <span>蜂群压力状态</span>
              <el-tag :type="beeStressAlarm.level" size="small" style="margin-left: auto;">
                {{ beeStressAlarm.text }}
              </el-tag>
            </div>
          </template>
          <div class="card-content">
            <div class="metric-value">
              <span class="value stress-value" :style="{ color: beeStressAlarm.color }">
                {{ beeStressAlarm.valueText }}
              </span>
            </div>
            <div class="metric-info">
              <p>模型标签: {{ dashboardStore.beeStressLabel || 'unknown' }}</p>
              <p>置信度: {{ dashboardStore.beeStressConfidence !== null ? (dashboardStore.beeStressConfidence * 100).toFixed(1) + '%' : '--' }}</p>
              <p>更新时间: {{ formatTime(dashboardStore.beeStressLastUpdate) }}</p>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><Monitor /></el-icon>
              <span>二氧化碳浓度</span>
            </div>
          </template>
          <div class="card-content">
            <div class="metric-value">
              <span class="value" :style="{ color: getCO2Color(dashboardStore.co2) }">
                {{ dashboardStore.co2 !== null ? dashboardStore.co2.toFixed(0) : '--' }}
              </span>
              <span class="unit">PPM</span>
            </div>
            <div class="metric-trend" v-if="dashboardStore.co2 !== null">
              <el-icon :color="dashboardStore.co2Trend > 0 ? '#f56c6c' : '#67c23a'">
                <ArrowUp v-if="dashboardStore.co2Trend > 0" />
                <ArrowDown v-else />
              </el-icon>
              <span :style="{ color: dashboardStore.co2Trend > 0 ? '#f56c6c' : '#67c23a' }">
                {{ dashboardStore.co2Trend > 0 ? '+' : '' }}{{ dashboardStore.co2Trend ? dashboardStore.co2Trend.toFixed(0) : 0 }}
              </span>
            </div>
            <div class="optimal-range">
              <span>正常范围: &lt;1000 PPM</span>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="status-card">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><DataAnalysis /></el-icon>
              <span>数据更新</span>
            </div>
          </template>
          <div class="card-content">
            <div class="metric-value">
              <span class="value">{{ dashboardStore.messageCount }}</span>
              <span class="unit">条</span>
            </div>
            <div class="metric-info">
              <p>最后更新: {{ formatTime(dashboardStore.lastUpdate) }}</p>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 环境趋势与设备状态 -->
    <el-row :gutter="20" class="chart-row">
      <el-col :xs="24" :lg="18">
        <el-card shadow="hover" class="chart-card environment-chart-card">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><PieChart /></el-icon>
              <span>环境趋势总览</span>
              <el-tag type="success" effect="plain" size="small" class="chart-range-tag">最近 1 分钟</el-tag>
            </div>
          </template>
          <div class="environment-chart-grid">
            <section class="trend-panel trend-panel-main">
              <div class="trend-panel-title"><span class="trend-dot temperature-dot"></span>温度与湿度</div>
              <div ref="temperatureChart" class="environment-chart environment-chart-main"></div>
            </section>
            <div class="trend-panel-stack">
              <section class="trend-panel">
                <div class="trend-panel-title"><span class="trend-dot co2-dot"></span>二氧化碳</div>
                <div ref="co2Chart" class="environment-chart environment-chart-small"></div>
              </section>
              <section class="trend-panel">
                <div class="trend-panel-title"><span class="trend-dot pressure-dot"></span>环境气压</div>
                <div ref="pressureChart" class="environment-chart environment-chart-small"></div>
              </section>
            </div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :lg="6">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><Histogram /></el-icon>
              <span>设备状态分布</span>
            </div>
          </template>
          <div class="chart-container">
            <div ref="statusChart" class="status-chart"></div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 最近消息 -->
    <el-row class="message-row">
      <el-col :span="24">
        <el-card shadow="hover" class="message-card">
          <template #header>
            <div class="card-header">
              <el-icon size="20"><Message /></el-icon>
              <span>最近MQTT消息</span>
            </div>
          </template>
          <el-table :data="dashboardStore.recentMessages" height="200" stripe>
            <el-table-column prop="timestamp" label="时间" width="180">
              <template #default="{ row }">
                {{ formatTime(row.timestamp) }}
              </template>
            </el-table-column>
            <el-table-column prop="topic" label="主题" width="200" />
            <el-table-column prop="message" label="消息内容" />
            <el-table-column label="操作" width="100">
              <template #default="{ row }">
                <el-button size="small" @click="handleMessageDetail(row)">详情</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch, computed } from 'vue'
import { useMQTTStore } from '@/stores/mqtt'
import { useDashboardStore } from '@/stores/dashboard'
import * as echarts from 'echarts'
import {
  Connection,
  Sunny,
  HotWater,
  DataAnalysis,
  PieChart,
  Histogram,
  Refresh,
  Message,
  ArrowUp,
  ArrowDown,
  TrendCharts,
  Warning,
  Monitor
} from '@element-plus/icons-vue'

// 状态管理
const mqttStore = useMQTTStore()
const dashboardStore = useDashboardStore()

// 图表引用
const temperatureChart = ref(null)
const co2Chart = ref(null)
const statusChart = ref(null)
const pressureChart = ref(null)

let temperatureChartInstance = null
let co2ChartInstance = null
let statusChartInstance = null
let pressureChartInstance = null

// 蜂箱环境报警阈值配置
const alarmConfig = {
  temperature: {
    low: 32,      // 蜂箱温度过低阈值：32°C（蜜蜂活动受影响）
    high: 38,     // 蜂箱温度过高阈值：38°C（蜂群热应激风险）
    unit: '°C',
    optimal: '34-36°C'  // 蜜蜂繁殖最佳温度
  },
  humidity: {
    low: 50,      // 蜂箱湿度过低阈值：50%（蜂蜜脱水过快）
    high: 75,    // 蜂箱湿度过高阈值：75%（霉菌和疾病风险）
    unit: '%',
    optimal: '50-70%'   // 最佳湿度范围
  }
}

// 蜂箱温度报警状态计算
const temperatureAlarm = computed(() => {
  const temp = dashboardStore.temperature
  if (temp === null || temp === undefined) {
    return { status: 'unknown', color: '#909399', text: '等待数据', level: 'info' }
  }
  if (temp < alarmConfig.temperature.low) {
    return { status: 'low', color: '#409EFF', text: '温度偏低', level: 'warning' }
  } else if (temp > alarmConfig.temperature.high) {
    return { status: 'high', color: '#F56C6C', text: '温度过高', level: 'danger' }
  } else if (temp >= 34 && temp <= 36) {
    return { status: 'optimal', color: '#67C23A', text: '最佳繁殖温度', level: 'success' }
  }
  return { status: 'normal', color: '#67C23A', text: '温度正常', level: 'success' }
})

// 蜂箱湿度报警状态计算
const humidityAlarm = computed(() => {
  const hum = dashboardStore.humidity
  if (hum === null || hum === undefined) {
    return { status: 'unknown', color: '#909399', text: '等待数据', level: 'info' }
  }
  if (hum < alarmConfig.humidity.low) {
    return { status: 'low', color: '#409EFF', text: '湿度偏低', level: 'warning' }
  } else if (hum > alarmConfig.humidity.high) {
    return { status: 'high', color: '#F56C6C', text: '湿度过高', level: 'danger' }
  } else if (hum >= 50 && hum <= 70) {
    return { status: 'optimal', color: '#67C23A', text: '最佳湿度', level: 'success' }
  }
  return { status: 'normal', color: '#67C23A', text: '湿度正常', level: 'success' }
})

const pressureAlarm = computed(() => {
  const pres = dashboardStore.pressure
  if (pres === null || pres === undefined) {
    return { status: 'unknown', color: '#909399', text: '等待数据', level: 'info' }
  }
  // 气压异常阈值（低于980hPa或高于1030hPa）
  if (pres < 980) {
    return { status: 'low', color: '#409EFF', text: '气压偏低', level: 'warning' }
  } else if (pres > 1030) {
    return { status: 'high', color: '#F56C6C', text: '气压偏高', level: 'danger' }
  }
  return { status: 'normal', color: '#67C23A', text: '气压正常', level: 'success' }
})

// 是否需要报警（用于显示警告图标）
const beeStressAlarm = computed(() => {
  if (dashboardStore.beeStressState === null || dashboardStore.beeStressState === undefined) {
    return { status: 'unknown', color: '#909399', text: '等待数据', valueText: '--', level: 'info' }
  }

  if (dashboardStore.beeStressState) {
    return { status: 'stress', color: '#F56C6C', text: '压力预警', valueText: '处于特殊情况', level: 'danger' }
  }

  return { status: 'normal', color: '#67C23A', text: '状态正常', valueText: '正常', level: 'success' }
})

const hasAlarm = computed(() => {
  return temperatureAlarm.value.status !== 'normal' || 
         humidityAlarm.value.status !== 'normal' || 
         pressureAlarm.value.status !== 'normal' ||
         beeStressAlarm.value.status === 'stress'
})

// 过滤最近1分钟的数据（30个点，每2秒一个点）
const recentMinuteData = computed(() => {
  const allData = dashboardStore.chartData
  const totalPoints = allData.timestamps.length
  const pointsToShow = Math.min(totalPoints, 30) // 1分钟数据
  
  const startIndex = totalPoints - pointsToShow
  
  return {
    timestamps: allData.timestamps.slice(startIndex),
    temperatures: allData.temperatures.slice(startIndex),
    humidities: allData.humidities.slice(startIndex),
    pressures: allData.pressures ? allData.pressures.slice(startIndex) : []
  }
})

// 格式化时间
const formatTime = (timestamp) => {
  if (!timestamp) return '--'
  const date = new Date(timestamp)
  return `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
}

// CO2颜色判断
const getCO2Color = (co2) => {
  if (co2 === null) return '#909399'
  if (co2 < 800) return '#67c23a' // 优秀
  if (co2 < 1000) return '#409eff' // 良好
  if (co2 < 1500) return '#e6a23c' // 警告
  return '#f56c6c' // 危险
}

// 连接MQTT
const handleConnectMQTT = () => {
  mqttStore.connect()
}

// 查看消息详情
const handleMessageDetail = (message) => {
  console.log('消息详情:', message)
}

// 初始化图表
const initTemperatureChart = () => {
  if (!temperatureChart.value) return
  
  temperatureChartInstance = echarts.init(temperatureChart.value)
  const option = {
    tooltip: {
      trigger: 'axis',
      formatter: function(params) {
        let result = `${params[0].axisValue}<br/>`
        params.forEach(item => {
          const value = item.value
          const formattedValue = typeof value === 'number' ? value.toFixed(2) : value
          const unit = item.seriesName === '温度' ? '°C' : '%'
          result += `${item.marker} ${item.seriesName}: ${formattedValue}${unit}<br/>`
        })
        return result
      }
    },
    legend: {
      data: ['温度', '湿度'],
      bottom: 0 // 将图例放在底部
    },
    grid: {
      left: '3%',
      right: '4%',
      top: '10%',
      bottom: '10%', // 为底部图例留出空间
      containLabel: true
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: recentMinuteData.value.timestamps
    },
    yAxis: [
      {
        type: 'value',
        name: '温度 (°C)',
        position: 'left'
      },
      {
        type: 'value',
        name: '湿度 (%)',
        position: 'right'
      }
    ],
    series: [
      {
        name: '温度',
        type: 'line',
        yAxisIndex: 0,
        data: recentMinuteData.value.temperatures,
        smooth: true,
        lineStyle: {
          color: '#ff6b6b'
        },
        itemStyle: {
          color: '#ff6b6b'
        }
      },
      {
        name: '湿度',
        type: 'line',
        yAxisIndex: 1,
        data: recentMinuteData.value.humidities,
        smooth: true,
        lineStyle: {
          color: '#4d96ff'
        },
        itemStyle: {
          color: '#4d96ff'
        }
      }
    ]
  }
  
  temperatureChartInstance.setOption(option)
}

const getStatusChartData = () => [
  { value: mqttStore.connected ? 1 : 0, name: 'MQTT已连接', itemStyle: { color: '#67C23A' } },
  { value: mqttStore.connected ? 0 : 1, name: 'MQTT未连接', itemStyle: { color: '#F56C6C' } },
  { value: dashboardStore.deviceOnline ? 1 : 0, name: '主ESP32在线', itemStyle: { color: '#409EFF' } },
  { value: mqttStore.connected && !dashboardStore.deviceOnline ? 1 : 0, name: '主ESP32离线', itemStyle: { color: '#E6A23C' } },
  { value: dashboardStore.edgeAiOnline ? 1 : 0, name: 'EdgeAI在线', itemStyle: { color: '#36CFC9' } },
  { value: mqttStore.connected && !dashboardStore.edgeAiOnline ? 1 : 0, name: 'EdgeAI离线', itemStyle: { color: '#909399' } }
]

const initStatusChart = () => {
  if (!statusChart.value) return
  
  statusChartInstance = echarts.init(statusChart.value)
  const option = {
    tooltip: {
      trigger: 'item'
    },
    legend: {
      type: 'scroll',
      orient: 'horizontal',
      left: 8,
      right: 8,
      bottom: 4,
      itemWidth: 10,
      itemHeight: 10,
      textStyle: { fontSize: 11 }
    },
    series: [
      {
        name: '设备状态',
        type: 'pie',
        radius: ['38%', '62%'],
        center: ['50%', '43%'],
        label: { show: false },
        labelLine: { show: false },
        data: [
          { value: mqttStore.connected ? 1 : 0, name: '已连接' },
          { value: mqttStore.connected ? 0 : 1, name: '未连接' },
          { value: dashboardStore.messageCount > 0 ? 1 : 0, name: '有数据' },
          { value: dashboardStore.messageCount === 0 ? 1 : 0, name: '无数据' }
        ],
        emphasis: {
          itemStyle: {
            shadowBlur: 10,
            shadowOffsetX: 0,
            shadowColor: 'rgba(0, 0, 0, 0.5)'
          }
        }
      }
    ]
  }
  
  statusChartInstance.setOption(option)
  statusChartInstance.setOption({
    series: [
      {
        name: '设备状态',
        data: getStatusChartData()
      }
    ]
  })
}

const initPressureChart = () => {
  if (!pressureChart.value) return
  
  pressureChartInstance = echarts.init(pressureChart.value)
  const option = {
    tooltip: {
      trigger: 'axis',
      formatter: function(params) {
        let result = `${params[0].axisValue}<br/>`
        params.forEach(item => {
          const value = item.value
          const formattedValue = typeof value === 'number' ? value.toFixed(2) : value
          const unit = 'hPa'
          result += `${item.marker} ${item.seriesName}: ${formattedValue}${unit}<br/>`
        })
        return result
      }
    },
    legend: {
      data: ['气压'],
      bottom: 0 // 将图例放在底部
    },
    grid: {
      left: '3%',
      right: '4%',
      top: '10%',
      bottom: '10%', // 为底部图例留出空间
      containLabel: true
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: recentMinuteData.value.timestamps
    },
    yAxis: {
      type: 'value',
      name: '气压 (hPa)',
      position: 'left'
    },
    series: [
      {
        name: '气压',
        type: 'line',
        data: recentMinuteData.value.pressures,
        smooth: true,
        lineStyle: {
          color: '#9b59b6'
        },
        itemStyle: {
          color: '#9b59b6'
        },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(155, 89, 182, 0.6)' },
            { offset: 1, color: 'rgba(155, 89, 182, 0.1)' }
          ])
        }
      }
    ]
  }
  
  pressureChartInstance.setOption(option)
}

const initCO2Chart = () => {
  if (!co2Chart.value) return
  
  co2ChartInstance = echarts.init(co2Chart.value)
  
  const chartData = dashboardStore.chartData
  const timestamps = chartData.timestamps.slice(-60)
  const co2Data = chartData.co2.slice(-60).map(v => v !== null ? v : '-')
  
  const option = {
    tooltip: {
      trigger: 'axis',
      formatter: function(params) {
        let result = `${params[0].axisValue}<br/>`
        params.forEach(item => {
          const value = item.value
          if (value !== '-' && value !== null && value !== undefined) {
            result += `${item.marker} CO₂: ${value} PPM<br/>`
          }
        })
        return result || '暂无数据'
      }
    },
    legend: {
      data: ['CO₂浓度'],
      bottom: 0
    },
    grid: {
      left: '3%',
      right: '4%',
      top: '10%',
      bottom: '15%',
      containLabel: true
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: timestamps
    },
    yAxis: {
      type: 'value',
      name: 'CO₂ (PPM)',
      position: 'left',
      min: 0,
      max: 'dataMax'
    },
    series: [
      {
        name: 'CO₂浓度',
        type: 'line',
        data: co2Data,
        smooth: true,
        lineStyle: {
          color: '#e6a23c'
        },
        itemStyle: {
          color: '#e6a23c'
        },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(230, 162, 60, 0.5)' },
            { offset: 1, color: 'rgba(230, 162, 60, 0.1)' }
          ])
        }
      }
    ]
  }
  
  co2ChartInstance.setOption(option)
}

// 监听窗口大小变化，重新调整图表
const handleResize = () => {
  if (temperatureChartInstance) {
    temperatureChartInstance.resize()
  }
  if (co2ChartInstance) {
    co2ChartInstance.resize()
  }
  if (statusChartInstance) {
    statusChartInstance.resize()
  }
  if (pressureChartInstance) {
    pressureChartInstance.resize()
  }
}

// 生命周期
onMounted(() => {
  nextTick(() => {
    initTemperatureChart()
    initStatusChart()
    initPressureChart()
    initCO2Chart()
    window.addEventListener('resize', handleResize)
  })
})

onUnmounted(() => {
  if (temperatureChartInstance) {
    temperatureChartInstance.dispose()
  }
  if (co2ChartInstance) {
    co2ChartInstance.dispose()
  }
  if (statusChartInstance) {
    statusChartInstance.dispose()
  }
  if (pressureChartInstance) {
    pressureChartInstance.dispose()
  }
  window.removeEventListener('resize', handleResize)
})

// 监听数据变化，更新图表
watch(() => dashboardStore.chartData, () => {
  if (temperatureChartInstance) {
    temperatureChartInstance.setOption({
      xAxis: {
        data: recentMinuteData.value.timestamps
      },
      series: [
        {
          data: recentMinuteData.value.temperatures
        },
        {
          data: recentMinuteData.value.humidities
        }
      ]
    })
  }
  
  if (statusChartInstance) {
    statusChartInstance.setOption({
      series: [
        {
          data: [
            { value: mqttStore.connected ? 1 : 0, name: '已连接' },
            { value: mqttStore.connected ? 0 : 1, name: '未连接' },
            { value: dashboardStore.messageCount > 0 ? 1 : 0, name: '有数据' },
            { value: dashboardStore.messageCount === 0 ? 1 : 0, name: '无数据' }
          ]
        }
      ]
    })
  }
  
  if (pressureChartInstance) {
    pressureChartInstance.setOption({
      xAxis: {
        data: recentMinuteData.value.timestamps
      },
      series: [
        {
          data: recentMinuteData.value.pressures
        }
      ]
    })
  }
  
  if (co2ChartInstance) {
    const chartData = dashboardStore.chartData
    const timestamps = chartData.timestamps.slice(-60)
    const co2Data = chartData.co2.slice(-60).map(v => v !== null ? v : '-')
    co2ChartInstance.setOption({
      xAxis: {
        data: timestamps
      },
      series: [
        {
          data: co2Data
        }
      ]
    })
  }
}, { deep: true })

watch(() => [
  mqttStore.connected,
  dashboardStore.deviceOnline,
  dashboardStore.edgeAiOnline,
  dashboardStore.messageCount
], () => {
  if (statusChartInstance) {
    statusChartInstance.setOption({
      series: [
        {
          name: '设备状态',
          data: getStatusChartData()
        }
      ]
    })
  }
})
</script>

<style scoped>
.dashboard-container {
  padding: 20px;
}

.status-cards {
  margin-bottom: 20px;
}

.status-card {
  height: 100%;
  transition: all 0.3s ease;
}

.status-card.alarm-card {
  border-left: 4px solid #e6a23c;
  animation: pulse-alarm 2s ease-in-out infinite;
}

@keyframes pulse-alarm {
  0%, 100% {
    box-shadow: 0 2px 12px 0 rgba(230, 162, 60, 0.3);
  }
  50% {
    box-shadow: 0 2px 20px 0 rgba(230, 162, 60, 0.5);
  }
}

.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: bold;
}

.card-content {
  padding: 10px 0;
}

.status-indicator {
  margin-bottom: 10px;
}

.status-info p {
  margin: 5px 0;
  font-size: 14px;
  color: #606266;
}

.metric-value {
  display: flex;
  align-items: baseline;
  margin-bottom: 10px;
}

.metric-value .value {
  font-size: 32px;
  font-weight: bold;
}

.metric-value .unit {
  font-size: 16px;
  color: #909399;
  margin-left: 5px;
}

.stress-value {
  font-size: 28px;
}

.metric-trend {
  display: flex;
  align-items: center;
  gap: 5px;
  font-size: 14px;
}

.metric-info p {
  margin: 5px 0;
  font-size: 14px;
  color: #606266;
}

.optimal-range {
  margin-top: 10px;
  padding-top: 8px;
  border-top: 1px dashed #e4e7ed;
  font-size: 12px;
  color: #909399;
}

.connect-button {
  margin-top: 12px;
}

.chart-row {
  margin-bottom: 20px;
}

.chart-card {
  height: 100%;
}

.chart-container {
  width: 100%;
}

.chart-range-tag {
  margin-left: auto;
}

.environment-chart-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.7fr) minmax(260px, 1fr);
  gap: 12px;
}

.trend-panel-stack {
  display: grid;
  min-width: 0;
  grid-template-rows: 1fr 1fr;
  gap: 12px;
}

.trend-panel {
  min-width: 0;
  padding: 12px;
  background: #f7faf8;
  border: 1px solid #e3ebe7;
  border-radius: 6px;
}

.trend-panel-title {
  display: flex;
  align-items: center;
  gap: 7px;
  color: #50615b;
  font-size: 12px;
  font-weight: 700;
}

.trend-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
}

.temperature-dot { background: #ff6b6b; }
.co2-dot { background: #d99520; }
.pressure-dot { background: #7967ad; }
.environment-chart { width: 100%; }
.environment-chart-main { height: 368px; }
.environment-chart-small { height: 160px; }
.status-chart { width: 100%; height: 420px; }

.message-row {
  margin-bottom: 20px;
}

.message-card {
  height: 100%;
}

/* 移动端适配 */
@media screen and (max-width: 768px) {
  .dashboard-container {
    padding: 12px;
  }
  
  .status-cards,
  .chart-row,
  .message-row {
    margin-bottom: 16px;
  }
  
  .metric-value .value {
    font-size: 24px;
  }
  
  .metric-value .unit {
    font-size: 14px;
  }
  
  .card-header {
    font-size: 14px;
  }
  
  .card-header .el-icon {
    font-size: 16px !important;
  }
  
  .status-info p,
  .metric-info p {
    font-size: 12px;
  }
  
  .chart-container div {
    height: 250px !important;
  }

  .environment-chart-grid {
    grid-template-columns: 1fr;
  }

  .trend-panel-stack {
    grid-template-rows: auto;
  }

  .environment-chart-main,
  .environment-chart-small {
    height: 240px !important;
  }

  .status-chart {
    height: 280px !important;
  }
}

/* 超小屏幕手机适配 */
@media screen and (max-width: 480px) {
  .dashboard-container {
    padding: 8px;
  }
  
  .metric-value .value {
    font-size: 20px;
  }
  
  .metric-value .unit {
    font-size: 12px;
  }
  
  .card-header {
    font-size: 13px;
    gap: 6px;
  }
  
  .chart-container div {
    height: 200px !important;
  }
}
</style>
