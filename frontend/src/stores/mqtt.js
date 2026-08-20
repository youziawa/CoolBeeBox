import { ref, computed } from 'vue'
import { defineStore } from 'pinia'
import mqtt from 'mqtt'

export const useMQTTStore = defineStore('mqtt', () => {
  // MQTT连接状态
  const connected = ref(false)
  const connecting = ref(false)
  const error = ref(null)
  
  // MQTT客户端实例
  const client = ref(null)
  
  // 连接配置
  const broker = ref('ws://1.94.161.42:8083/mqtt') // EMQX默认WebSocket端口
  const username = ref('admin')
  const password = ref('')
  const clientId = ref(`web_client_${Math.random().toString(16).substr(2, 8)}`)
  const deviceId = ref('esp32_device_001')
  
  // 订阅主题列表
  const subscriptions = ref(new Set())
  
  // 连接选项
  const connectionOptions = computed(() => ({
    username: username.value,
    password: password.value,
    clientId: clientId.value,
    clean: true,
    reconnectPeriod: 5000, // 5秒重连间隔
    connectTimeout: 4000 // 4秒连接超时
  }))
  
  // 连接状态文本
  const statusText = computed(() => {
    if (connecting.value) return '连接中...'
    if (connected.value) return '已连接'
    if (error.value) return `连接失败: ${error.value}`
    return '未连接'
  })
  
  // 连接MQTT Broker
  const connect = () => {
    if (connecting.value || connected.value) {
      disconnect()
    }
    
    connecting.value = true
    error.value = null
    
    try {
      console.log(`正在连接MQTT Broker: ${broker.value}`)
      
      client.value = mqtt.connect(broker.value, connectionOptions.value)
      
      // 连接成功回调
      client.value.on('connect', () => {
        console.log('MQTT连接成功')
        connected.value = true
        connecting.value = false
        error.value = null
        
        // 触发连接状态变化事件
        window.dispatchEvent(new CustomEvent('mqtt-connection-change', {
          detail: { connected: true }
        }))
        
        // 自动订阅设备数据主题
        subscribe(`device/${deviceId.value}/data`)
        subscribe(`device/${deviceId.value}/status`)
        subscribe(`device/${deviceId.value}/control`)
        
        // 订阅ESP32传感器数据（包含温度、湿度、气压、CO2等）
        subscribe('device/coolbee/sensors')
        subscribe('device/coolbee/edgeai')
        subscribe('device/coolbee/edgeai/status')
      })
      
      // 连接关闭回调
      client.value.on('close', () => {
        console.log('MQTT连接关闭')
        connected.value = false
        connecting.value = false
        
        // 触发连接状态变化事件
        window.dispatchEvent(new CustomEvent('mqtt-connection-change', {
          detail: { connected: false }
        }))
      })
      
      // 错误回调
      client.value.on('error', (err) => {
        console.error('MQTT连接错误:', err)
        error.value = err.message
        connected.value = false
        connecting.value = false
      })
      
      // 消息到达回调
      client.value.on('message', (topic, message) => {
        console.log(`收到MQTT消息: ${topic} - ${message.toString()}`)
        
        // 触发消息事件，供其他组件监听
        const messageEvent = new CustomEvent('mqtt-message', {
          detail: {
            topic,
            message: message.toString(),
            timestamp: Date.now()
          }
        })
        window.dispatchEvent(messageEvent)
      })
      
      // 重连回调
      client.value.on('reconnect', () => {
        console.log('MQTT正在重连...')
        connecting.value = true
      })
      
    } catch (err) {
      console.error('MQTT连接异常:', err)
      error.value = err.message
      connecting.value = false
    }
  }
  
  // 断开MQTT连接
  const disconnect = () => {
    if (client.value) {
      try {
        // 取消所有订阅
        subscriptions.value.forEach(topic => {
          client.value.unsubscribe(topic)
        })
        subscriptions.value.clear()
        
        // 关闭连接
        client.value.end(true)
        console.log('MQTT连接已断开')
      } catch (err) {
        console.error('断开MQTT连接时出错:', err)
      } finally {
        client.value = null
        connected.value = false
        connecting.value = false
      }
    }
  }
  
  // 订阅主题
  const subscribe = (topic) => {
    if (!client.value || !connected.value) {
      console.warn('MQTT客户端未连接，无法订阅主题')
      return false
    }
    
    try {
      client.value.subscribe(topic, { qos: 0 }, (err) => {
        if (err) {
          console.error(`订阅主题 ${topic} 失败:`, err)
        } else {
          console.log(`成功订阅主题: ${topic}`)
          subscriptions.value.add(topic)
        }
      })
      return true
    } catch (err) {
      console.error(`订阅主题 ${topic} 异常:`, err)
      return false
    }
  }
  
  // 取消订阅
  const unsubscribe = (topic) => {
    if (!client.value || !connected.value) {
      console.warn('MQTT客户端未连接，无法取消订阅')
      return false
    }
    
    try {
      client.value.unsubscribe(topic, (err) => {
        if (err) {
          console.error(`取消订阅主题 ${topic} 失败:`, err)
        } else {
          console.log(`成功取消订阅主题: ${topic}`)
          subscriptions.value.delete(topic)
        }
      })
      return true
    } catch (err) {
      console.error(`取消订阅主题 ${topic} 异常:`, err)
      return false
    }
  }
  
  // 发布消息
  const publish = (topic, message, options = { qos: 0, retain: false }) => {
    if (!client.value || !connected.value) {
      console.warn('MQTT客户端未连接，无法发布消息')
      return false
    }
    
    try {
      const payload = typeof message === 'object' ? JSON.stringify(message) : String(message)
      client.value.publish(topic, payload, options, (err) => {
        if (err) {
          console.error(`发布消息到 ${topic} 失败:`, err)
        } else {
          console.log(`成功发布消息到 ${topic}: ${payload}`)
        }
      })
      return true
    } catch (err) {
      console.error(`发布消息到 ${topic} 异常:`, err)
      return false
    }
  }
  
  // 发送设备控制命令
  const sendControlCommand = (command, value = null) => {
    const controlTopic = `device/${deviceId.value}/control`
    const payload = value !== null ? { command, value } : { command }
    return publish(controlTopic, payload)
  }
  
  // 更新连接配置
  const updateConfig = (config) => {
    if (config.broker !== undefined) broker.value = config.broker
    if (config.username !== undefined) username.value = config.username
    if (config.password !== undefined) password.value = config.password
    if (config.deviceId !== undefined) deviceId.value = config.deviceId
    if (config.clientId !== undefined) clientId.value = config.clientId
    
    // 如果已连接，重新连接
    if (connected.value) {
      disconnect()
      setTimeout(() => connect(), 1000)
    }
  }
  
  // 清理函数
  const cleanup = () => {
    disconnect()
  }
  
  return {
    // 状态
    connected,
    connecting,
    error,
    broker,
    username,
    password,
    clientId,
    deviceId,
    subscriptions,
    
    // 计算属性
    connectionOptions,
    statusText,
    
    // 方法
    connect,
    disconnect,
    subscribe,
    unsubscribe,
    publish,
    sendControlCommand,
    updateConfig,
    cleanup
  }
})
