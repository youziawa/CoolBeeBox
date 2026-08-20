<template>
  <div class="realtime-container">
    <!-- 页面标题和连接状态 -->
    <div class="page-header">
      <div class="connection-status">
        <el-tag :type="mqttStore.connected ? 'success' : 'danger'" size="large">
          {{ mqttStore.statusText }}
        </el-tag>
        <el-button
          v-if="!mqttStore.connected"
          type="primary"
          size="small"
          :icon="Connection"
          :loading="mqttStore.connecting"
          @click="mqttStore.connect"
        >
          连接EMQX
        </el-button>
        <el-button
          v-else
          type="warning"
          size="small"
          :icon="Close"
          @click="mqttStore.disconnect"
        >
          断开连接
        </el-button>
      </div>
    </div>

    <!-- 实时数据卡片 -->
    <el-row :gutter="20" class="realtime-cards">
      <el-col :xs="24" :sm="12" :md="8" :lg="6">
        <el-card shadow="hover" class="realtime-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Sunny /></el-icon>
              <span>当前温度</span>
            </div>
          </template>
          <div class="realtime-value">
            <span class="value">{{ dashboardStore.temperature.toFixed(2) }}</span>
            <span class="unit">°C</span>
          </div>
          <div class="realtime-trend">
            <el-icon :color="dashboardStore.temperatureTrend > 0 ? '#f56c6c' : '#67c23a'">
              <ArrowUp v-if="dashboardStore.temperatureTrend > 0" />
              <ArrowDown v-else />
            </el-icon>
            <span :style="{ color: dashboardStore.temperatureTrend > 0 ? '#f56c6c' : '#67c23a' }">
              {{ dashboardStore.temperatureTrend > 0 ? '+' : '' }}{{ dashboardStore.temperatureTrend.toFixed(2) }}°C
            </span>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="8" :lg="6">
        <el-card shadow="hover" class="realtime-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><HotWater /></el-icon>
              <span>当前湿度</span>
            </div>
          </template>
          <div class="realtime-value">
            <span class="value">{{ dashboardStore.humidity.toFixed(2) }}</span>
            <span class="unit">%</span>
          </div>
          <div class="realtime-trend">
            <el-icon :color="dashboardStore.humidityTrend > 0 ? '#f56c6c' : '#67c23a'">
              <ArrowUp v-if="dashboardStore.humidityTrend > 0" />
              <ArrowDown v-else />
            </el-icon>
            <span :style="{ color: dashboardStore.humidityTrend > 0 ? '#f56c6c' : '#67c23a' }">
              {{ dashboardStore.humidityTrend > 0 ? '+' : '' }}{{ dashboardStore.humidityTrend.toFixed(2) }}%
            </span>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="8" :lg="6">
        <el-card shadow="hover" class="realtime-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><DataLine /></el-icon>
              <span>消息频率</span>
            </div>
          </template>
          <div class="realtime-value">
            <span class="value">{{ messageRate }}</span>
            <span class="unit">条/分钟</span>
          </div>
          <div class="realtime-info">
            <p>总计: {{ dashboardStore.messageCount }} 条</p>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="8" :lg="6">
        <el-card shadow="hover" class="realtime-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Monitor /></el-icon>
              <span>二氧化碳浓度</span>
            </div>
          </template>
          <div class="realtime-value">
            <span class="value">{{ dashboardStore.co2 !== null ? dashboardStore.co2.toFixed(0) : '--' }}</span>
            <span class="unit">PPM</span>
          </div>
          <div class="realtime-trend" v-if="dashboardStore.co2 !== null">
            <el-icon :color="dashboardStore.co2Trend > 0 ? '#f56c6c' : '#67c23a'">
              <ArrowUp v-if="dashboardStore.co2Trend > 0" />
              <ArrowDown v-else />
            </el-icon>
            <span :style="{ color: dashboardStore.co2Trend > 0 ? '#f56c6c' : '#67c23a' }">
              {{ dashboardStore.co2Trend > 0 ? '+' : '' }}{{ dashboardStore.co2Trend ? dashboardStore.co2Trend.toFixed(0) : 0 }}
            </span>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="8" :lg="6">
        <el-card shadow="hover" class="realtime-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Clock /></el-icon>
              <span>最后更新</span>
            </div>
          </template>
          <div class="realtime-value">
            <span class="value">{{ lastUpdateTime }}</span>
          </div>
          <div class="realtime-info">
            <p>{{ lastUpdateDate }}</p>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 实时图表 -->
    <el-row :gutter="20" class="chart-section">
      <el-col :xs="24" :md="12" :xl="6">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><TrendCharts /></el-icon>
              <span>温度实时趋势</span>
              <div class="chart-controls">
                <el-button-group size="small">
                  <el-button
                    :type="timeRange === '5m' ? 'primary' : 'default'"
                    @click="timeRange = '5m'"
                  >
                    5分钟
                  </el-button>
                  <el-button
                    :type="timeRange === '15m' ? 'primary' : 'default'"
                    @click="timeRange = '15m'"
                  >
                    15分钟
                  </el-button>
                  <el-button
                    :type="timeRange === '30m' ? 'primary' : 'default'"
                    @click="timeRange = '30m'"
                  >
                    30分钟
                  </el-button>
                </el-button-group>
              </div>
            </div>
          </template>
          <div class="chart-container">
            <div ref="temperatureRealtimeChart" style="width: 100%; height: 300px;"></div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :md="12" :xl="6">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><PieChart /></el-icon>
              <span>湿度实时趋势</span>
            </div>
          </template>
          <div class="chart-container">
            <div ref="humidityRealtimeChart" style="width: 100%; height: 300px;"></div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :md="12" :xl="6">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><TrendCharts /></el-icon>
              <span>气压实时趋势</span>
              <div class="chart-controls">
                <el-button-group size="small">
                  <el-button
                    :type="timeRange === '5m' ? 'primary' : 'default'"
                    @click="timeRange = '5m'"
                  >
                    5分钟
                  </el-button>
                  <el-button
                    :type="timeRange === '15m' ? 'primary' : 'default'"
                    @click="timeRange = '15m'"
                  >
                    15分钟
                  </el-button>
                  <el-button
                    :type="timeRange === '30m' ? 'primary' : 'default'"
                    @click="timeRange = '30m'"
                  >
                    30分钟
                  </el-button>
                </el-button-group>
              </div>
            </div>
          </template>
          <div class="chart-container">
            <div ref="pressureRealtimeChart" style="width: 100%; height: 300px;"></div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :md="12" :xl="6">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Monitor /></el-icon>
              <span>CO₂ 实时趋势</span>
            </div>
          </template>
          <div class="chart-container">
            <div ref="co2RealtimeChart" style="width: 100%; height: 300px;"></div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 实时消息流 -->
    <el-row class="message-section">
      <el-col :span="24">
        <el-card shadow="hover" class="message-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Message /></el-icon>
              <span>实时消息流</span>
              <div class="message-controls">
                <el-switch
                  v-model="autoScroll"
                  active-text="自动滚动"
                  inactive-text="暂停滚动"
                />
                <el-button
                  size="small"
                  :icon="Delete"
                  @click="clearMessages"
                >
                  清空消息
                </el-button>
              </div>
            </div>
          </template>
          <div class="message-container" ref="messageContainer">
            <div
              v-for="message in dashboardStore.recentMessages"
              :key="message.id"
              class="message-item"
              :class="getMessageClass(message)"
            >
              <div class="message-time">{{ formatTime(message.timestamp) }}</div>
              <div class="message-topic">{{ message.topic }}</div>
              <div class="message-content">{{ message.message }}</div>
            </div>
            <div v-if="dashboardStore.recentMessages.length === 0" class="empty-message">
              暂无消息，请连接EMQX并开始数据流
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- MQTT主题订阅管理 -->
    <el-row class="subscription-section">
      <el-col :span="24">
        <el-card shadow="hover" class="subscription-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Connection /></el-icon>
              <span>MQTT主题订阅管理</span>
            </div>
          </template>
          <div class="subscription-content">
            <div class="subscription-list">
              <div class="subscription-header">
                <span>已订阅主题</span>
                <el-button
                  size="small"
                  type="primary"
                  :icon="Plus"
                  @click="showAddSubscription = true"
                >
                  添加订阅
                </el-button>
              </div>
              <el-table
                :data="Array.from(mqttStore.subscriptions)"
                height="200"
                stripe
                empty-text="暂无订阅的主题"
              >
                <el-table-column prop="topic" label="主题" />
                <el-table-column label="操作" width="120">
                  <template #default="{ row }">
                    <el-button
                      size="small"
                      type="danger"
                      :icon="Close"
                      @click="unsubscribeTopic(row)"
                    >
                      取消订阅
                    </el-button>
                  </template>
                </el-table-column>
              </el-table>
            </div>
            
            <div class="topic-suggestions">
              <h4>常用主题建议</h4>
              <div class="suggestion-list">
                <el-tag
                  v-for="topic in suggestedTopics"
                  :key="topic"
                  class="topic-tag"
                  @click="subscribeToTopic(topic)"
                >
                  {{ topic }}
                </el-tag>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 添加订阅对话框 -->
    <el-dialog
      v-model="showAddSubscription"
      title="添加MQTT订阅"
      width="500px"
    >
      <el-form :model="subscriptionForm" label-width="80px">
        <el-form-item label="主题">
          <el-input
            v-model="subscriptionForm.topic"
            placeholder="例如: device/+/data"
          />
        </el-form-item>
        <el-form-item label="QoS等级">
          <el-radio-group v-model="subscriptionForm.qos">
            <el-radio :label="0">0 - 最多一次</el-radio>
            <el-radio :label="1">1 - 至少一次</el-radio>
            <el-radio :label="2">2 - 恰好一次</el-radio>
          </el-radio-group>
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="showAddSubscription = false">取消</el-button>
          <el-button type="primary" @click="addSubscription">确认订阅</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, computed, watch, nextTick } from 'vue'
import { useMQTTStore } from '@/stores/mqtt'
import { useDashboardStore } from '@/stores/dashboard'
import * as echarts from 'echarts'
import {
  Connection,
  Close,
  Sunny,
  HotWater,
  DataLine,
  Clock,
  TrendCharts,
  PieChart,
  Refresh,
  Delete,
  Message,
  ArrowUp,
  ArrowDown,
  Plus,
  Monitor
} from '@element-plus/icons-vue'

// 状态管理
const mqttStore = useMQTTStore()
const dashboardStore = useDashboardStore()

// 图表引用
const temperatureRealtimeChart = ref(null)
const humidityRealtimeChart = ref(null)
const pressureRealtimeChart = ref(null)
const co2RealtimeChart = ref(null)
let temperatureChartInstance = null
let humidityChartInstance = null
let pressureChartInstance = null
let co2ChartInstance = null

// 组件状态
const timeRange = ref('15m')
const showHumidityGradient = ref(true)
const autoScroll = ref(true)
const showAddSubscription = ref(false)
const autoScale = ref(true)

// 订阅表单
const subscriptionForm = ref({
  topic: '',
  qos: 0
})

// 建议主题
const suggestedTopics = ref([
  'device/+/data',
  'device/+/status',
  'device/+/control',
  'sensors/temperature',
  'sensors/humidity',
  'esp32/#'
])

// 计算属性
const messageRate = computed(() => {
  const recentCount = dashboardStore.recentMessages.length
  // 假设消息在最近5分钟内到达
  return Math.round(recentCount / 5)
})

// 根据时间范围过滤图表数据
const filteredChartData = computed(() => {
  const allData = dashboardStore.chartData
  const totalPoints = allData.timestamps.length
  
  let pointsToShow = totalPoints
  if (timeRange.value === '5m') {
    pointsToShow = Math.min(totalPoints, 150) // 5分钟数据（每2秒一个点）
  } else if (timeRange.value === '15m') {
    pointsToShow = Math.min(totalPoints, 450) // 15分钟数据
  } // '30m' 显示全部数据
  
  const startIndex = totalPoints - pointsToShow
  
  console.log(`时间范围: ${timeRange.value}, 总点数: ${totalPoints}, 显示点数: ${pointsToShow}, 起始索引: ${startIndex}`)
  
  return {
    timestamps: allData.timestamps.slice(startIndex),
    temperatures: allData.temperatures.slice(startIndex),
    humidities: allData.humidities.slice(startIndex),
    pressures: allData.pressures ? allData.pressures.slice(startIndex) : [],
    co2: allData.co2 ? allData.co2.slice(startIndex).map(value => value ?? '-') : []
  }
})

const lastUpdateTime = computed(() => {
  const date = new Date(dashboardStore.lastUpdate)
  return `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
})

const lastUpdateDate = computed(() => {
  const date = new Date(dashboardStore.lastUpdate)
  return `${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')}`
})

// 方法
const formatTime = (timestamp) => {
  const date = new Date(timestamp)
  return `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
}

const getMessageClass = (message) => {
  if (message.topic.includes('data')) return 'message-data'
  if (message.topic.includes('status')) return 'message-status'
  if (message.topic.includes('control')) return 'message-control'
  return 'message-other'
}

const refreshCharts = () => {
  if (temperatureChartInstance) {
    temperatureChartInstance.clear()
    initTemperatureChart()
  }
  if (humidityChartInstance) {
    humidityChartInstance.clear()
    initHumidityChart()
  }
  if (pressureChartInstance) {
    pressureChartInstance.clear()
    initPressureChart()
  }
  if (co2ChartInstance) {
    co2ChartInstance.clear()
    initCO2Chart()
  }
}

const clearData = () => {
  dashboardStore.resetData()
  refreshCharts()
}

const clearMessages = () => {
  dashboardStore.recentMessages = []
}

const unsubscribeTopic = (topic) => {
  mqttStore.unsubscribe(topic)
}

const subscribeToTopic = (topic) => {
  mqttStore.subscribe(topic)
  showAddSubscription.value = false
}

const addSubscription = () => {
  if (subscriptionForm.value.topic.trim()) {
    subscribeToTopic(subscriptionForm.value.topic.trim())
    subscriptionForm.value.topic = ''
  }
}

// 消息容器自动滚动
const messageContainer = ref(null)
watch(() => dashboardStore.recentMessages, () => {
  if (autoScroll.value) {
    nextTick(() => {
      if (messageContainer.value) {
        messageContainer.value.scrollTop = messageContainer.value.scrollHeight
      }
    })
  }
}, { deep: true })

// 图表初始化
const initTemperatureChart = () => {
  if (!temperatureRealtimeChart.value) return
  
  temperatureChartInstance = echarts.init(temperatureRealtimeChart.value)
  
  const option = {
    tooltip: {
      trigger: 'axis',
      formatter: function(params) {
        const date = new Date()
        const time = `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
        let result = `${time}<br/>`
        params.forEach(item => {
          const value = item.value
          const formattedValue = typeof value === 'number' ? value.toFixed(2) : value
          result += `${item.marker} ${item.seriesName}: ${formattedValue}°C<br/>`
        })
        return result
      }
    },
    grid: {
      left: '3%',
      right: '4%',
      bottom: '3%',
      containLabel: true
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: filteredChartData.value.timestamps
    },
    yAxis: {
      type: 'value',
      name: '温度 (°C)',
      min: autoScale.value ? null : 15,
      max: autoScale.value ? null : 35
    },
    series: [
      {
        name: '温度',
        type: 'line',
        data: filteredChartData.value.temperatures,
        smooth: true,
        lineStyle: {
          width: 3,
          color: '#ff6b6b'
        },
        itemStyle: {
          color: '#ff6b6b'
        },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(255, 107, 107, 0.6)' },
            { offset: 1, color: 'rgba(255, 107, 107, 0.1)' }
          ])
        }
      }
    ]
  }
  
  temperatureChartInstance.setOption(option)
}

const initHumidityChart = () => {
  if (!humidityRealtimeChart.value) return
  
  humidityChartInstance = echarts.init(humidityRealtimeChart.value)
  
  const gradient = showHumidityGradient.value
    ? new echarts.graphic.LinearGradient(0, 0, 0, 1, [
        { offset: 0, color: 'rgba(77, 150, 255, 0.8)' },
        { offset: 1, color: 'rgba(77, 150, 255, 0.1)' }
      ])
    : '#4d96ff'
  
  const option = {
    tooltip: {
      trigger: 'axis',
      formatter: function(params) {
        const date = new Date()
        const time = `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
        let result = `${time}<br/>`
        params.forEach(item => {
          const value = item.value
          const formattedValue = typeof value === 'number' ? value.toFixed(2) : value
          result += `${item.marker} ${item.seriesName}: ${formattedValue}%<br/>`
        })
        return result
      }
    },
    grid: {
      left: '3%',
      right: '4%',
      bottom: '3%',
      containLabel: true
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: filteredChartData.value.timestamps
    },
    yAxis: {
      type: 'value',
      name: '湿度 (%)',
      min: autoScale.value ? null : 20,
      max: autoScale.value ? null : 90
    },
    series: [
      {
        name: '湿度',
        type: 'line',
        data: filteredChartData.value.humidities,
        smooth: true,
        lineStyle: {
          width: 3,
          color: '#4d96ff'
        },
        itemStyle: {
          color: '#4d96ff'
        },
        areaStyle: {
          color: gradient
        }
      }
    ]
  }
  
  humidityChartInstance.setOption(option)
}

const initPressureChart = () => {
  if (!pressureRealtimeChart.value) return
  
  pressureChartInstance = echarts.init(pressureRealtimeChart.value)
  
  const option = {
    tooltip: {
      trigger: 'axis',
      formatter: function(params) {
        const date = new Date()
        const time = `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
        let result = `${time}<br/>`
        params.forEach(item => {
          const value = item.value
          const formattedValue = typeof value === 'number' ? value.toFixed(2) : value
          result += `${item.marker} ${item.seriesName}: ${formattedValue}hPa<br/>`
        })
        return result
      }
    },
    grid: {
      left: '3%',
      right: '4%',
      bottom: '3%',
      containLabel: true
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: filteredChartData.value.timestamps
    },
    yAxis: {
      type: 'value',
      name: '气压 (hPa)',
      min: autoScale.value ? null : 980,
      max: autoScale.value ? null : 1030
    },
    series: [
      {
        name: '气压',
        type: 'line',
        data: filteredChartData.value.pressures,
        smooth: true,
        lineStyle: {
          width: 3,
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
  if (!co2RealtimeChart.value) return

  co2ChartInstance = echarts.init(co2RealtimeChart.value)
  co2ChartInstance.setOption({
    tooltip: {
      trigger: 'axis',
      valueFormatter: value => value === '-' ? '暂无数据' : `${Number(value).toFixed(0)} PPM`
    },
    grid: { left: '3%', right: '4%', bottom: '3%', containLabel: true },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: filteredChartData.value.timestamps
    },
    yAxis: {
      type: 'value',
      name: 'CO₂ (PPM)',
      min: 0,
      max: 'dataMax'
    },
    series: [{
      name: 'CO₂',
      type: 'line',
      data: filteredChartData.value.co2,
      smooth: true,
      connectNulls: false,
      lineStyle: { width: 3, color: '#d99520' },
      itemStyle: { color: '#d99520' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(217, 149, 32, 0.48)' },
          { offset: 1, color: 'rgba(217, 149, 32, 0.06)' }
        ])
      }
    }]
  })
}

// 监听数据变化，更新图表
watch(() => dashboardStore.chartData, () => {
  if (temperatureChartInstance) {
    temperatureChartInstance.setOption({
      xAxis: {
        data: filteredChartData.value.timestamps
      },
      series: [
        {
          data: filteredChartData.value.temperatures
        }
      ]
    })
  }
  
  if (humidityChartInstance) {
    humidityChartInstance.setOption({
      xAxis: {
        data: filteredChartData.value.timestamps
      },
      series: [
        {
          data: filteredChartData.value.humidities
        }
      ]
    })
  }
  
  if (pressureChartInstance) {
    pressureChartInstance.setOption({
      xAxis: {
        data: filteredChartData.value.timestamps
      },
      series: [
        {
          data: filteredChartData.value.pressures
        }
      ]
    })
  }
  if (co2ChartInstance) {
    co2ChartInstance.setOption({
      xAxis: { data: filteredChartData.value.timestamps },
      series: [{ data: filteredChartData.value.co2 }]
    })
  }
}, { deep: true })

// 监听过滤数据变化（时间范围变化时触发）
watch(filteredChartData, () => {
  if (temperatureChartInstance) {
    temperatureChartInstance.setOption({
      xAxis: {
        data: filteredChartData.value.timestamps
      },
      series: [
        {
          data: filteredChartData.value.temperatures
        }
      ]
    })
  }
  
  if (humidityChartInstance) {
    humidityChartInstance.setOption({
      xAxis: {
        data: filteredChartData.value.timestamps
      },
      series: [
        {
          data: filteredChartData.value.humidities
        }
      ]
    })
  }
  
  if (pressureChartInstance) {
    pressureChartInstance.setOption({
      xAxis: {
        data: filteredChartData.value.timestamps
      },
      series: [
        {
          data: filteredChartData.value.pressures
        }
      ]
    })
  }
  if (co2ChartInstance) {
    co2ChartInstance.setOption({
      xAxis: { data: filteredChartData.value.timestamps },
      series: [{ data: filteredChartData.value.co2 }]
    })
  }
}, { deep: true })

// 监听配置变化
watch(autoScale, () => {
  refreshCharts()
})

// 监听时间范围变化
watch(timeRange, () => {
  refreshCharts()
})

// 窗口大小调整处理
const handleResize = () => {
  if (temperatureChartInstance) {
    temperatureChartInstance.resize()
  }
  if (humidityChartInstance) {
    humidityChartInstance.resize()
  }
  if (pressureChartInstance) {
    pressureChartInstance.resize()
  }
  if (co2ChartInstance) {
    co2ChartInstance.resize()
  }
}

// 生命周期
onMounted(() => {
  nextTick(() => {
    initTemperatureChart()
    initHumidityChart()
    initPressureChart()
    initCO2Chart()
    window.addEventListener('resize', handleResize)
  })
})

onUnmounted(() => {
  if (temperatureChartInstance) {
    temperatureChartInstance.dispose()
  }
  if (humidityChartInstance) {
    humidityChartInstance.dispose()
  }
  if (pressureChartInstance) {
    pressureChartInstance.dispose()
  }
  if (co2ChartInstance) {
    co2ChartInstance.dispose()
  }
  window.removeEventListener('resize', handleResize)
})
</script>

<style scoped>
.realtime-container {
  padding: 20px;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.page-header h2 {
  margin: 0;
}

.connection-status {
  display: flex;
  align-items: center;
  gap: 10px;
}

.realtime-cards {
  margin-bottom: 20px;
}

.realtime-card {
  height: 100%;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: bold;
}

.realtime-value {
  display: flex;
  align-items: baseline;
  margin-bottom: 10px;
}

.realtime-value .value {
  font-size: 28px;
  font-weight: bold;
  color: #1a1a1a;
}

.realtime-value .unit {
  font-size: 16px;
  color: #404040;
  margin-left: 5px;
}

.realtime-trend {
  display: flex;
  align-items: center;
  gap: 5px;
  font-size: 14px;
  color: #303030;
}

.realtime-info p {
  margin: 5px 0;
  font-size: 14px;
  color: #303030;
}

.chart-section {
  margin-bottom: 20px;
}

.chart-card {
  height: 100%;
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.chart-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.chart-container {
  width: 100%;
}

.control-section {
  margin-bottom: 20px;
}

.control-card {
  height: 100%;
}

.control-content {
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.control-buttons {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.control-config {
  background-color: #f5f7fa;
  padding: 15px;
  border-radius: 4px;
}

.message-section {
  margin-bottom: 20px;
}

.message-card {
  height: 100%;
}

.message-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.message-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.message-container {
  max-height: 300px;
  overflow-y: auto;
  border: 1px solid #ebeef5;
  border-radius: 4px;
  padding: 10px;
}

.message-item {
  padding: 8px;
  border-bottom: 1px solid #f0f0f0;
  font-family: monospace;
}

.message-item:last-child {
  border-bottom: none;
}

.message-item.message-data {
  background-color: rgba(64, 158, 255, 0.05);
}

.message-item.message-status {
  background-color: rgba(103, 194, 58, 0.05);
}

.message-item.message-control {
  background-color: rgba(230, 162, 60, 0.05);
}

.message-item.message-other {
  background-color: rgba(144, 147, 153, 0.05);
}

.message-time {
  font-size: 12px;
  color: #909399;
}

.message-topic {
  font-weight: bold;
  color: #409eff;
  margin: 2px 0;
}

.message-content {
  font-size: 14px;
  color: #606266;
}

.empty-message {
  text-align: center;
  color: #909399;
  padding: 20px;
}

.subscription-section {
  margin-bottom: 20px;
}

.subscription-card {
  height: 100%;
}

.subscription-content {
  display: flex;
  gap: 20px;
}

.subscription-list {
  flex: 1;
}

.subscription-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
}

.topic-suggestions {
  flex: 1;
}

.suggestion-list {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 10px;
}

.topic-tag {
  cursor: pointer;
}

.topic-tag:hover {
  background-color: #409eff;
  color: white;
}

/* 移动端适配 */
@media screen and (max-width: 768px) {
  .realtime-container {
    padding: 12px;
  }
  
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
    margin-bottom: 16px;
  }
  
  .connection-status {
    width: 100%;
    justify-content: space-between;
  }
  
  .realtime-cards,
  .chart-section,
  .control-section,
  .message-section,
  .subscription-section {
    margin-bottom: 16px;
  }
  
  .realtime-value .value {
    font-size: 22px;
  }
  
  .realtime-value .unit {
    font-size: 14px;
  }
  
  .card-header {
    font-size: 14px;
  }
  
  .card-header .el-icon {
    font-size: 16px !important;
  }
  
  .realtime-info p {
    font-size: 12px;
  }
  
  .chart-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 10px;
  }
  
  .chart-controls {
    width: 100%;
    justify-content: center;
  }
  
  .chart-controls .el-button-group {
    width: 100%;
    display: flex;
  }
  
  .chart-controls .el-button-group .el-button {
    flex: 1;
    font-size: 12px;
    padding: 6px 8px;
  }
  
  .control-buttons {
    gap: 8px;
  }
  
  .control-buttons .el-button {
    flex: 1;
    min-width: 140px;
  }
  
  .control-content {
    gap: 12px;
  }
  
  .control-config {
    padding: 12px;
  }
  
  .message-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 10px;
  }
  
  .message-controls {
    width: 100%;
    justify-content: space-between;
  }
  
  .message-container {
    max-height: 200px;
  }
  
  .subscription-content {
    flex-direction: column;
    gap: 15px;
  }
  
  .suggestion-list {
    justify-content: center;
  }
  
  /* 图表高度调整 */
  .chart-container div {
    height: 250px !important;
  }
}

/* 超小屏幕手机适配 */
@media screen and (max-width: 480px) {
  .realtime-container {
    padding: 8px;
  }
  
  .realtime-value .value {
    font-size: 20px;
  }
  
  .realtime-value .unit {
    font-size: 12px;
  }
  
  .card-header {
    font-size: 13px;
    gap: 6px;
  }
  
  .chart-controls .el-button-group .el-button {
    font-size: 11px;
    padding: 5px 6px;
  }
  
  .control-buttons .el-button {
    min-width: 120px;
    font-size: 12px;
    padding: 8px 12px;
  }
  
  .message-container {
    max-height: 180px;
  }
  
  /* 图表高度调整 */
  .chart-container div {
    height: 200px !important;
  }
}
</style>
