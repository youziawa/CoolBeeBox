import { ref, computed, watch } from 'vue'
import { defineStore } from 'pinia'
import { useMQTTStore } from './mqtt'

export const useDashboardStore = defineStore('dashboard', () => {
  // 实时数据
  const temperature = ref(25.5)
  const humidity = ref(65.8)
  const pressure = ref(1013.2) // 气压，单位hPa
  const co2 = ref(null) // 二氧化碳浓度，单位PPM
  const gas = ref(null) // 气体阻力，单位Ω
  const beeStressState = ref(null)
  const beeStressConfidence = ref(null)
  const beeStressLabel = ref('unknown')
  const beeStressLastUpdate = ref(null)
  const temperatureTrend = ref(0.2)
  const humidityTrend = ref(-0.5)
  const pressureTrend = ref(0.5) // 气压趋势
  const co2Trend = ref(0) // CO2趋势
  
  // 数据统计
  const messageCount = ref(0)
  const lastUpdate = ref(Date.now())
  
  // 设备在线状态检测
  const deviceOnline = ref(false) // 设备是否在线
  const lastDeviceUptime = ref(null) // 上一个有效的设备uptime
  const deviceOfflineTimeout = ref(30000) // 设备离线超时时间（毫秒）
  let deviceOfflineCheckTimer = null // 设备离线检查定时器
  
  // 数据来源追踪
  const edgeAiOnline = ref(false)
  const lastEdgeAiUptime = ref(null)
  const edgeAiOfflineTimeout = ref(60000)
  let edgeAiOfflineCheckTimer = null

  const dataSource = ref('none') // 'none' | 'esp32'
  
  // 最近消息列表
  const recentMessages = ref([])
  const maxRecentMessages = 20
  
  // 图表数据
  const chartData = ref({
    timestamps: [],
    temperatures: [],
    humidities: [],
    pressures: [],
    co2: [],
    uptimes: [] // ESP32设备运行时间
  })
  
  const maxChartPoints = 450 // 最多存储15分钟数据（每2秒一个点）
  
  // MQTT store引用
  const mqttStore = useMQTTStore()
  
  // 计算属性：连接状态
  const isConnected = computed(() => mqttStore.connected)
  
  // 添加消息到最近消息列表
  const addMessage = (topic, message) => {
    const timestamp = Date.now()
    const newMessage = {
      timestamp,
      topic,
      message,
      id: `${timestamp}_${Math.random().toString(16).substr(2, 8)}`
    }
    
    recentMessages.value.unshift(newMessage)
    
    // 保持列表不超过最大长度
    if (recentMessages.value.length > maxRecentMessages) {
      recentMessages.value = recentMessages.value.slice(0, maxRecentMessages)
    }
    
    // 更新统计
    messageCount.value++
    lastUpdate.value = timestamp
  }
  
  // 添加数据点到图表
  const addChartDataPoint = (temp, hum, pres = null, co2Value = null, uptime = null) => {
    const now = new Date()
    const timestamp = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`
    
    chartData.value.timestamps.push(timestamp)
    chartData.value.temperatures.push(temp)
    chartData.value.humidities.push(hum)
    // 如果有气压数据则添加，否则使用当前气压值
    chartData.value.pressures.push(pres !== null ? pres : pressure.value)
    // 添加CO2数据
    chartData.value.co2.push(co2Value)
    // 添加设备uptime
    chartData.value.uptimes.push(uptime)
    
    // 保持图表数据不超过最大点数
    if (chartData.value.timestamps.length > maxChartPoints) {
      chartData.value.timestamps.shift()
      chartData.value.temperatures.shift()
      chartData.value.humidities.shift()
      chartData.value.pressures.shift()
      chartData.value.co2.shift()
      chartData.value.uptimes.shift()
    }
  }

  const toBooleanStressState = (value) => {
    if (typeof value === 'boolean') return value
    if (typeof value === 'number') return value > 0
    if (typeof value === 'string') {
      const normalized = value.trim().toLowerCase()
      if (['stress', 'stressed', 'warning', 'alert', '1', 'true', 'yes'].includes(normalized)) return true
      if (['normal', 'ok', 'healthy', '0', 'false', 'no'].includes(normalized)) return false
    }
    return null
  }

  const updateBeeStressState = (data, topic) => {
    edgeAiOnline.value = true
    lastEdgeAiUptime.value = data.uptime !== undefined ? parseInt(data.uptime) : lastEdgeAiUptime.value
    resetEdgeAiOfflineCheck()

    const label = data.label ?? data.class ?? data.state ?? data.status ?? data.result ?? data.prediction ?? data.raw
    const stressValue = data.stress ?? data.is_stress ?? data.bee_stress ?? data.stressed ?? label
    const nextStressState = toBooleanStressState(stressValue)

    if (nextStressState === null) {
      addMessage(topic, `EdgeAI data: ${JSON.stringify(data)}`)
      return
    }

    const confidenceValue = data.confidence ?? data.score ?? data.probability ?? data.value
    const parsedConfidence = confidenceValue !== undefined ? parseFloat(confidenceValue) : null

    beeStressState.value = nextStressState
    beeStressConfidence.value = Number.isFinite(parsedConfidence) ? parsedConfidence : null
    beeStressLabel.value = label ? String(label) : (nextStressState ? 'stress' : 'normal')
    beeStressLastUpdate.value = Date.now()

    const confidenceInfo = beeStressConfidence.value !== null ? ` (${(beeStressConfidence.value * 100).toFixed(1)}%)` : ''
    addMessage(topic, `EdgeAI bee state: ${beeStressState.value ? 'stress' : 'normal'}${confidenceInfo}`)
  }
  
  // 解析MQTT消息
  const parseMQTTMessage = (topic, message) => {
    try {
      // 尝试解析JSON消息
      let data
      try {
        data = JSON.parse(message)
      } catch {
        // 如果不是JSON，尝试其他格式
        data = { raw: message }
      }
      
      // 根据主题处理不同类型的数据
      if (topic.toLowerCase().includes('edgeai') || topic.toLowerCase().includes('/ai') || data.stress !== undefined || data.is_stress !== undefined || data.bee_stress !== undefined) {
        updateBeeStressState(data, topic)
      } else if (topic.includes('/data') || topic.includes('bme680') || topic.includes('sensor') || topic.includes('sensors')) {
        // 解析ESP32设备uptime（秒）
        const deviceUptime = data.uptime !== undefined ? parseInt(data.uptime) : null
        
        // 标记数据来源为ESP32真实数据
        dataSource.value = 'esp32'
        
        // 如果有uptime字段，进行数据有效性检查
        if (deviceUptime !== null) {
          // 检查uptime是否有效（应该递增）
          if (lastDeviceUptime.value !== null) {
            const uptimeDiff = deviceUptime - lastDeviceUptime.value
            
            // 如果uptime差值为负或为0，说明设备可能重启了
            if (uptimeDiff <= 0) {
              console.warn('检测到设备可能重启: uptime从', lastDeviceUptime.value, '变为', deviceUptime)
              addMessage(topic, `⚠️ 设备重启检测: uptime ${lastDeviceUptime.value}s → ${deviceUptime}s`)
            }
            // 如果uptime间隔超过10秒，说明设备可能有较长的离线或数据延迟
            else if (uptimeDiff > 10) {
              console.warn('检测到设备数据间隔异常: uptime间隔', uptimeDiff, '秒')
              addMessage(topic, `⚠️ 数据延迟警告: uptime间隔 ${uptimeDiff}秒`)
            }
          }
          
          // 更新最后的设备uptime
          lastDeviceUptime.value = deviceUptime
        }
        
        // 标记设备在线并重置离线检测计时器
        deviceOnline.value = true
        resetDeviceOfflineCheck()
        
        // 设备数据消息
        const newTemp = parseFloat(data.temperature) || parseFloat(data.temp) || temperature.value + (Math.random() - 0.5) * 0.5
        const newHum = parseFloat(data.humidity) || parseFloat(data.hum) || humidity.value + (Math.random() - 0.5) * 2
        const newPres = parseFloat(data.pressure) || parseFloat(data.pres) || pressure.value + (Math.random() - 0.5) * 1
        const newCo2 = data.co2 !== undefined ? parseFloat(data.co2) : null
        const newGas = data.gas !== undefined ? parseFloat(data.gas) : null
        
        console.log('[parseMQTTMessage] 收到数据:', JSON.stringify(data))
        console.log('[parseMQTTMessage] co2值:', newCo2, 'gas值:', newGas)
        
        // 更新趋势
        temperatureTrend.value = newTemp - temperature.value
        humidityTrend.value = newHum - humidity.value
        pressureTrend.value = newPres - pressure.value
        if (co2.value !== null) {
          if (newCo2 !== null) {
            co2Trend.value = newCo2 - co2.value
          }
        }
        
        // 更新当前值
        temperature.value = newTemp
        humidity.value = newHum
        pressure.value = newPres
        if (newCo2 !== null) {
          co2.value = newCo2
          console.log('[parseMQTTMessage] CO2已更新:', co2.value)
        }
        if (newGas !== null) gas.value = newGas
        
        // 添加到图表（包含uptime）
        addChartDataPoint(newTemp, newHum, newPres, newCo2, deviceUptime)
        
        const uptimeInfo = deviceUptime !== null ? ` (设备运行: ${Math.floor(deviceUptime / 3600)}h ${Math.floor((deviceUptime % 3600) / 60)}m ${deviceUptime % 60}s)` : ''
        const co2Info = newCo2 !== null ? `, CO₂ ${newCo2.toFixed(0)}PPM` : ''
        addMessage(topic, `🌡️ ESP32数据: 温度 ${newTemp.toFixed(2)}°C, 湿度 ${newHum.toFixed(2)}%, 气压 ${newPres.toFixed(1)}hPa${co2Info}${uptimeInfo}`)
        
      } else if (topic.includes('/status')) {
        // 设备状态消息
        addMessage(topic, `设备状态: ${JSON.stringify(data)}`)
        
      } else if (topic.includes('/control')) {
        // 控制响应消息
        addMessage(topic, `控制响应: ${JSON.stringify(data)}`)
        
      } else {
        // 其他消息
        addMessage(topic, message)
      }
      
    } catch (err) {
      console.error('解析MQTT消息时出错:', err)
      addMessage(topic, `解析错误: ${message}`)
    }
  }
  
  // 监听MQTT消息
  const setupMQTTListener = () => {
    console.log('[setupMQTTListener] 开始设置MQTT消息监听器')
    
    const handleMQTTMessage = (event) => {
      console.log('[setupMQTTListener] 收到mqtt-message事件:', event.detail)
      const { topic, message, timestamp } = event.detail
      parseMQTTMessage(topic, message)
    }
    
    // 监听MQTT连接状态变化
    const handleConnectionChange = (event) => {
      console.log('[setupMQTTListener] 收到mqtt-connection-change事件:', event.detail)
      if (!event.detail.connected) {
        // MQTT断开时标记设备离线
        setDeviceOffline()
        console.log('MQTT连接断开，标记设备离线')
      }
    }
    
    window.addEventListener('mqtt-message', handleMQTTMessage)
    window.addEventListener('mqtt-connection-change', handleConnectionChange)
    
    console.log('[setupMQTTListener] MQTT消息监听器设置完成')
    
    // 返回清理函数
    return () => {
      window.removeEventListener('mqtt-message', handleMQTTMessage)
      window.removeEventListener('mqtt-connection-change', handleConnectionChange)
    }
  }
  
  // 重置设备离线检测计时器
  const resetDeviceOfflineCheck = () => {
    // 清除现有的离线检测定时器
    if (deviceOfflineCheckTimer) {
      clearTimeout(deviceOfflineCheckTimer)
    }
    
    // 设置新的离线检测定时器
    deviceOfflineCheckTimer = setTimeout(() => {
      checkDeviceOffline()
    }, deviceOfflineTimeout.value)
  }
  
  // 检查设备是否离线
  const checkDeviceOffline = () => {
    if (deviceOnline.value) {
      console.log('设备离线检测: 未在', deviceOfflineTimeout.value / 1000, '秒内收到数据，标记设备离线')
      deviceOnline.value = false
      dataSource.value = 'none'
      addMessage('system', '⚠️ 设备已离线: 超过30秒未收到ESP32数据')
    }
  }
  
  // 设置设备离线状态
  const resetEdgeAiOfflineCheck = () => {
    if (edgeAiOfflineCheckTimer) {
      clearTimeout(edgeAiOfflineCheckTimer)
    }

    edgeAiOfflineCheckTimer = setTimeout(() => {
      checkEdgeAiOffline()
    }, edgeAiOfflineTimeout.value)
  }

  const checkEdgeAiOffline = () => {
    if (edgeAiOnline.value) {
      edgeAiOnline.value = false
      addMessage('system', 'EdgeAI ESP32 offline: no EdgeAI data for 60 seconds')
    }
  }

  const setDeviceOffline = () => {
    deviceOnline.value = false
    edgeAiOnline.value = false
    if (dataSource.value === 'esp32') {
      dataSource.value = 'none'
    }
    if (deviceOfflineCheckTimer) {
      clearTimeout(deviceOfflineCheckTimer)
      deviceOfflineCheckTimer = null
    }
    if (edgeAiOfflineCheckTimer) {
      clearTimeout(edgeAiOfflineCheckTimer)
      edgeAiOfflineCheckTimer = null
    }
  }
  
  // 重置数据
  const resetData = () => {
    temperature.value = 25.5
    humidity.value = 65.8
    pressure.value = 1013.2
    co2.value = null
    gas.value = null
    beeStressState.value = null
    beeStressConfidence.value = null
    beeStressLabel.value = 'unknown'
    beeStressLastUpdate.value = null
    temperatureTrend.value = 0.2
    humidityTrend.value = -0.5
    pressureTrend.value = 0.5
    co2Trend.value = 0
    messageCount.value = 0
    lastUpdate.value = Date.now()
    recentMessages.value = []
    deviceOnline.value = false
    edgeAiOnline.value = false
    lastDeviceUptime.value = null
    lastEdgeAiUptime.value = null
    dataSource.value = 'none'
    if (edgeAiOfflineCheckTimer) {
      clearTimeout(edgeAiOfflineCheckTimer)
      edgeAiOfflineCheckTimer = null
    }
    chartData.value = {
      timestamps: [],
      temperatures: [],
      humidities: [],
      pressures: [],
      co2: [],
      uptimes: []
    }
  }
  
  // 获取设备控制命令历史
  const getControlHistory = () => {
    return recentMessages.value.filter(msg => 
      msg.topic.includes('/control') || msg.message.includes('控制命令')
    )
  }
  
  // 获取数据统计
  const getStatistics = () => {
    const tempData = chartData.value.temperatures
    const humData = chartData.value.humidities
    const presData = chartData.value.pressures
    
    return {
      temperature: {
        current: temperature.value,
        avg: tempData.length > 0 ? tempData.reduce((a, b) => a + b, 0) / tempData.length : 0,
        min: tempData.length > 0 ? Math.min(...tempData) : 0,
        max: tempData.length > 0 ? Math.max(...tempData) : 0,
        trend: temperatureTrend.value
      },
      humidity: {
        current: humidity.value,
        avg: humData.length > 0 ? humData.reduce((a, b) => a + b, 0) / humData.length : 0,
        min: humData.length > 0 ? Math.min(...humData) : 0,
        max: humData.length > 0 ? Math.max(...humData) : 0,
        trend: humidityTrend.value
      },
      pressure: {
        current: pressure.value,
        avg: presData.length > 0 ? presData.reduce((a, b) => a + b, 0) / presData.length : 0,
        min: presData.length > 0 ? Math.min(...presData) : 0,
        max: presData.length > 0 ? Math.max(...presData) : 0,
        trend: pressureTrend.value
      },
      messages: {
        total: messageCount.value,
        recent: recentMessages.value.length,
        lastUpdate: lastUpdate.value
      }
    }
  }
  
  // 发送控制命令
  const sendControlCommand = (command, value = null) => {
    if (!mqttStore.connected) {
      console.warn('MQTT未连接，无法发送控制命令')
      return false
    }
    
    const result = mqttStore.sendControlCommand(command, value)
    if (result) {
      addMessage('control/out', `发送控制命令: ${command}${value !== null ? ` = ${value}` : ''}`)
    }
    return result
  }
  
  // 初始化
  const initialize = () => {
    // 设置MQTT消息监听器
    const cleanupListener = setupMQTTListener()
    
    // 返回清理函数
    return cleanupListener
  }
  
  // 导出
  return {
    // 状态
    temperature,
    humidity,
    pressure,
    co2,
    gas,
    beeStressState,
    beeStressConfidence,
    beeStressLabel,
    beeStressLastUpdate,
    temperatureTrend,
    humidityTrend,
    pressureTrend,
    co2Trend,
    messageCount,
    lastUpdate,
    recentMessages,
    chartData,
    deviceOnline,
    edgeAiOnline,
    lastDeviceUptime,
    lastEdgeAiUptime,
    dataSource,
    
    // 计算属性
    isConnected,
    
    // 方法
    addMessage,
    addChartDataPoint,
    parseMQTTMessage,
    resetData,
    getControlHistory,
    getStatistics,
    sendControlCommand,
    setDeviceOffline,
    initialize
  }
})
