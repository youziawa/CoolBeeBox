<template>
  <div class="ai-container">
    <div class="page-header">
      <div class="header-actions">
        <el-button 
          type="primary" 
          size="large" 
          @click="showApiKeyDialog = true"
          :icon="Key"
        >
          配置API密钥
        </el-button>
      </div>
    </div>

    <el-tabs v-model="activeTab" class="ai-tabs">
      <!-- 报告生成标签 -->
      <el-tab-pane label="蜂箱报告" name="report">
        <div class="report-section">
          <el-card shadow="hover" class="report-intro">
            <template #header>
              <div class="card-header">
                <el-icon size="20"><Document /></el-icon>
                <span>今日蜂箱健康报告</span>
              </div>
            </template>
            <div class="report-content">
              <p class="intro-text">
                基于最近24小时的传感器数据，AI将为您生成一份详细的蜂箱健康分析报告。
              </p>
              <div class="report-features">
                <div class="feature-item">
                  <el-icon color="var(--primary-color)"><Check /></el-icon>
                  <span>温湿度趋势分析</span>
                </div>
                <div class="feature-item">
                  <el-icon color="var(--primary-color)"><Check /></el-icon>
                  <span>异常检测与预警</span>
                </div>
                <div class="feature-item">
                  <el-icon color="var(--primary-color)"><Check /></el-icon>
                  <span>养护建议</span>
                </div>
                <div class="feature-item">
                  <el-icon color="var(--primary-color)"><Check /></el-icon>
                  <span>健康度评分</span>
                </div>
              </div>
              <el-button 
                type="primary" 
                size="large" 
                :loading="reportLoading"
                @click="generateReport"
                class="generate-btn"
              >
                <el-icon v-if="!reportLoading"><MagicStick /></el-icon>
                {{ reportLoading ? '正在生成报告...' : '生成报告' }}
              </el-button>
            </div>
          </el-card>

          <el-card v-if="reportData" shadow="hover" class="report-result">
            <template #header>
              <div class="card-header">
                <el-icon size="20"><SuccessFilled /></el-icon>
                <span>报告已生成</span>
                <el-tag type="success" style="margin-left: 10px;">
                  {{ reportData.dataCount }} 条数据
                </el-tag>
              </div>
            </template>
            <div class="report-display">
              <div class="report-text" v-html="formatReport(reportData.report)"></div>
              <div class="report-actions">
                <el-button @click="copyReport" :icon="DocumentCopy">
                  复制报告
                </el-button>
                <el-button @click="generateReport" :icon="Refresh" :loading="reportLoading">
                  重新生成
                </el-button>
              </div>
            </div>
          </el-card>

          <el-empty 
            v-if="!reportData && !reportLoading" 
            description="点击上方按钮生成今日蜂箱报告"
            :image-size="120"
          />
        </div>
      </el-tab-pane>

      <!-- AI对话标签 -->
      <el-tab-pane label="AI 对话" name="chat">
        <div class="chat-section">
          <el-card shadow="hover" class="chat-container">
            <template #header>
              <div class="card-header">
                <el-icon size="20"><ChatDotRound /></el-icon>
                <span>蜂博士 - AI养蜂助手</span>
                <el-button 
                  text 
                  size="small" 
                  @click="clearChat"
                  style="margin-left: auto;"
                >
                  清空对话
                </el-button>
              </div>
            </template>
            
            <div class="chat-messages" ref="chatContainer">
              <div v-if="messages.length === 0" class="chat-welcome">
                <el-icon class="welcome-icon"><Service /></el-icon>
                <h3>你好！我是蜂博士</h3>
                <p>我可以帮你分析蜂箱数据，也可以回答养蜂相关的问题。有什么需要帮助的吗？</p>
                <div class="suggestion-chips">
                  <el-tag 
                    v-for="suggestion in suggestions" 
                    :key="suggestion"
                    class="suggestion-chip"
                    @click="sendSuggestion(suggestion)"
                  >
                    {{ suggestion }}
                  </el-tag>
                </div>
              </div>

              <div 
                v-for="(msg, index) in messages" 
                :key="index"
                :class="['message', msg.role]"
              >
                <div class="message-avatar">
                  <el-avatar :size="36" :icon="msg.role === 'user' ? User : Service" />
                </div>
                <div class="message-content">
                  <div class="message-header">
                    <span class="message-name">{{ msg.role === 'user' ? '你' : '蜂博士' }}</span>
                    <span class="message-time">{{ formatTime(msg.timestamp) }}</span>
                  </div>
                  <div class="message-text" v-html="msg.role === 'assistant' ? marked(msg.content) : msg.content"></div>
                </div>
              </div>

              <div v-if="chatLoading" class="message ai loading">
                <div class="message-avatar">
                  <el-avatar :size="36" :icon="Service" />
                </div>
                <div class="message-content">
                  <div class="message-header">
                    <span class="message-name">蜂博士</span>
                  </div>
                  <div class="message-text">
                    <el-icon class="is-loading"><Loading /></el-icon>
                    <span style="margin-left: 8px;">正在思考中...</span>
                  </div>
                </div>
              </div>
            </div>

            <div class="chat-input">
              <el-input
                v-model="inputMessage"
                type="textarea"
                :rows="2"
                placeholder="输入你的问题... (Shift+Enter换行，Enter发送)"
                @keydown.enter.exact.prevent="sendMessage"
                :disabled="chatLoading || !hasApiKey"
              />
              <el-button 
                type="primary" 
                :disabled="!inputMessage.trim() || chatLoading || !hasApiKey"
                @click="sendMessage"
                :loading="chatLoading"
              >
                <el-icon><Promotion /></el-icon>
                发送
              </el-button>
            </div>
          </el-card>

          <div v-if="!hasApiKey" class="api-key-warning">
            <el-alert
              title="请先配置DeepSeek API密钥"
              type="warning"
              description="点击右上角「配置API密钥」按钮进行配置"
              show-icon
              :closable="false"
            />
          </div>
        </div>
      </el-tab-pane>
    </el-tabs>

    <!-- API密钥配置对话框 -->
    <el-dialog
      v-model="showApiKeyDialog"
      title="配置 DeepSeek API密钥"
      width="500px"
    >
      <el-form>
        <el-form-item label="API密钥">
          <el-input
            v-model="tempApiKey"
            type="password"
            placeholder="请输入DeepSeek API密钥"
            show-password
          />
        </el-form-item>
        <el-alert
          type="info"
          :closable="false"
          style="margin-top: 10px;"
        >
          <template #title>
            <span>获取API密钥：</span>
            <a href="https://platform.deepseek.com/" target="_blank" style="color: #409eff;">
              https://platform.deepseek.com/
            </a>
          </template>
        </el-alert>
      </el-form>
      <template #footer>
        <el-button @click="showApiKeyDialog = false">取消</el-button>
        <el-button type="primary" @click="saveApiKey">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, computed } from 'vue'
import { marked } from 'marked'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Document,
  MagicStick,
  Refresh,
  SuccessFilled,
  DocumentCopy,
  ChatDotRound,
  User,
  Promotion,
  Loading,
  Check,
  Key,
  Service
} from '@element-plus/icons-vue'

// 配置marked选项
marked.setOptions({
  breaks: true,
  gfm: true
})

const API_BASE_URL = 'http://1.94.161.42:3000'

const activeTab = ref('report')
const reportLoading = ref(false)
const reportData = ref(null)
const chatLoading = ref(false)
const messages = ref([])
const inputMessage = ref('')
const chatContainer = ref(null)
const showApiKeyDialog = ref(false)
const tempApiKey = ref('')
const hasApiKey = ref(false)

const suggestions = [
  '蜂箱温度保持在多少度最合适？',
  '湿度对蜜蜂有什么影响？',
  '如何判断蜂群是否健康？',
  '夏天蜂箱通风要注意什么？',
  '蜜蜂常见疾病有哪些？'
]

onMounted(() => {
  loadApiKey()
})

const loadApiKey = () => {
  const savedKey = localStorage.getItem('deepseek_api_key')
  hasApiKey.value = !!savedKey
  if (savedKey) {
    tempApiKey.value = savedKey
  }
}

const saveApiKey = () => {
  if (!tempApiKey.value.trim()) {
    ElMessage.warning('请输入API密钥')
    return
  }
  
  localStorage.setItem('deepseek_api_key', tempApiKey.value.trim())
  hasApiKey.value = true
  showApiKeyDialog.value = false
  ElMessage.success('API密钥已保存')
}

const generateReport = async () => {
  if (!hasApiKey.value) {
    ElMessage.warning('请先配置API密钥')
    showApiKeyDialog.value = true
    return
  }

  reportLoading.value = true
  reportData.value = null

  try {
    const response = await fetch(`${API_BASE_URL}/api/ai/report`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    })

    const result = await response.json()

    if (result.success) {
      reportData.value = result.data
      ElMessage.success('报告生成成功！')
    } else {
      ElMessage.error(result.error || '生成报告失败')
    }
  } catch (error) {
    console.error('生成报告失败:', error)
    ElMessage.error('网络错误，请检查后端服务是否正常运行')
  } finally {
    reportLoading.value = false
  }
}

const formatReport = (report) => {
  if (!report) return ''
  return marked(report)
}

const copyReport = async () => {
  if (!reportData.value?.report) return
  
  try {
    await navigator.clipboard.writeText(reportData.value.report)
    ElMessage.success('报告已复制到剪贴板')
  } catch (err) {
    ElMessage.error('复制失败')
  }
}

const sendMessage = async () => {
  if (!inputMessage.value.trim() || chatLoading.value) return

  const userMessage = inputMessage.value.trim()
  inputMessage.value = ''

  messages.value.push({
    role: 'user',
    content: userMessage,
    timestamp: new Date()
  })

  scrollToBottom()
  chatLoading.value = true

  try {
    const response = await fetch(`${API_BASE_URL}/api/ai/chat`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        message: userMessage
      })
    })

    const result = await response.json()

    if (result.success) {
      messages.value.push({
        role: 'assistant',
        content: result.data.message,
        timestamp: new Date(result.data.timestamp)
      })
    } else {
      ElMessage.error(result.error || 'AI响应失败')
    }
  } catch (error) {
    console.error('发送消息失败:', error)
    ElMessage.error('网络错误，请检查后端服务是否正常运行')
  } finally {
    chatLoading.value = false
    scrollToBottom()
  }
}

const sendSuggestion = (suggestion) => {
  inputMessage.value = suggestion
  sendMessage()
}

const clearChat = () => {
  if (messages.value.length > 0) {
    ElMessageBox.confirm('确定要清空所有对话记录吗？', '提示', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    }).then(() => {
      messages.value = []
      ElMessage.success('对话已清空')
    }).catch(() => {})
  }
}

const formatTime = (timestamp) => {
  const date = new Date(timestamp)
  const now = new Date()
  const isToday = date.toDateString() === now.toDateString()
  
  if (isToday) {
    return date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
  }
  return date.toLocaleString('zh-CN', { 
    month: 'short', 
    day: 'numeric',
    hour: '2-digit',
    minute: '2-digit'
  })
}

const scrollToBottom = () => {
  nextTick(() => {
    if (chatContainer.value) {
      chatContainer.value.scrollTop = chatContainer.value.scrollHeight
    }
  })
}
</script>

<style scoped>
.ai-container {
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

.header-actions {
  display: flex;
  gap: 10px;
}

.ai-tabs {
  background: #fff;
  padding: 20px;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.report-section {
  max-width: 900px;
  margin: 0 auto;
}

.report-intro {
  margin-bottom: 20px;
}

.intro-text {
  color: #606266;
  line-height: 1.8;
  margin-bottom: 20px;
}

.report-features {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
  margin-bottom: 25px;
}

.feature-item {
  display: flex;
  align-items: center;
  gap: 8px;
  color: #303030;
}

.generate-btn {
  width: 100%;
  height: 50px;
  font-size: 16px;
}

.report-result {
  margin-top: 20px;
}

.report-display {
  padding: 10px 0;
}

.report-text {
  background: #f5f7fa;
  padding: 20px;
  border-radius: 8px;
  line-height: 1.8;
  color: #303030;
  margin-bottom: 20px;
  max-height: 500px;
  overflow-y: auto;
}

.report-text :deep(h1),
.report-text :deep(h2),
.report-text :deep(h3),
.report-text :deep(h4) {
  margin-top: 16px;
  margin-bottom: 8px;
  color: #1a1a1a;
}

.report-text :deep(p) {
  margin: 10px 0;
}

.report-text :deep(ul),
.report-text :deep(ol) {
  padding-left: 24px;
  margin: 10px 0;
}

.report-text :deep(li) {
  margin: 6px 0;
}

.report-text :deep(strong) {
  color: #1a1a1a;
  font-weight: 600;
}

.report-text :deep(code) {
  background: #ebeef5;
  padding: 2px 6px;
  border-radius: 4px;
  font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
  font-size: 13px;
}

.report-text :deep(blockquote) {
  border-left: 4px solid var(--primary-color);
  padding-left: 16px;
  margin: 12px 0;
  color: #606266;
  background: #f0f9ff;
  padding: 12px 16px;
  border-radius: 4px;
}

.report-text :deep(hr) {
  border: none;
  border-top: 1px solid #e4e7ed;
  margin: 16px 0;
}

.report-actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
}

.chat-section {
  max-width: 900px;
  margin: 0 auto;
}

.chat-container {
  height: 600px;
  display: flex;
  flex-direction: column;
}

.chat-messages {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
  background: #f5f7fa;
  border-radius: 8px;
  margin-bottom: 15px;
}

.chat-welcome {
  text-align: center;
  padding: 40px 20px;
  color: #606266;
}

.welcome-icon {
  font-size: 64px;
  margin-bottom: 20px;
}

.chat-welcome h3 {
  margin: 0 0 15px;
  color: #303030;
  font-size: 24px;
}

.chat-welcome p {
  margin: 0 0 25px;
  line-height: 1.8;
}

.suggestion-chips {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  justify-content: center;
}

.suggestion-chip {
  cursor: pointer;
  transition: all 0.3s;
}

.suggestion-chip:hover {
  transform: scale(1.05);
}

.message {
  display: flex;
  gap: 15px;
  margin-bottom: 20px;
}

.message.user {
  flex-direction: row-reverse;
}

.message-content {
  max-width: 75%;
}

.message-header {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 5px;
}

.message.user .message-header {
  flex-direction: row-reverse;
}

.message-name {
  font-weight: 600;
  color: #303030;
}

.message-time {
  font-size: 12px;
  color: #909399;
}

.message-text {
  padding: 12px 16px;
  border-radius: 12px;
  line-height: 1.6;
  color: #1a1a1a;
}

.message.user .message-text {
  background: var(--primary-color);
  color: #fff;
}

.message.ai .message-text {
  background: #fff;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.message.ai .message-text :deep(p) {
  margin: 8px 0;
}

.message.ai .message-text :deep(p:first-child) {
  margin-top: 0;
}

.message.ai .message-text :deep(p:last-child) {
  margin-bottom: 0;
}

.message.ai .message-text :deep(ul),
.message.ai .message-text :deep(ol) {
  padding-left: 20px;
  margin: 8px 0;
}

.message.ai .message-text :deep(li) {
  margin: 4px 0;
}

.message.ai .message-text :deep(code) {
  background: #f5f7fa;
  padding: 2px 6px;
  border-radius: 4px;
  font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
  font-size: 13px;
}

.message.ai .message-text :deep(strong) {
  color: #1a1a1a;
}

.chat-input {
  display: flex;
  gap: 10px;
  align-items: flex-end;
}

.chat-input .el-textarea {
  flex: 1;
}

.api-key-warning {
  margin-top: 15px;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 600;
}

@media screen and (max-width: 768px) {
  .ai-container {
    padding: 12px;
  }
  
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 15px;
    margin-bottom: 15px;
  }
  
  .report-features {
    grid-template-columns: 1fr;
  }
  
  .chat-container {
    height: min(560px, calc(100dvh - 250px));
    min-height: 420px;
  }
  
  .message-content {
    max-width: 85%;
  }
  
  .suggestion-chips {
    align-items: stretch;
    flex-direction: column;
  }

  .suggestion-chip {
    height: auto;
    padding: 8px 10px;
    white-space: normal;
  }

  .chat-input {
    gap: 8px;
  }

  .chat-input :deep(.el-button) {
    width: 44px;
    padding: 8px;
  }
}
</style>
