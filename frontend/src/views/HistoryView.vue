<template>
  <div class="history-container">
    <div class="page-header">
      <div class="header-actions">
        <el-button type="primary" :icon="Download" @click="exportData">
          导出数据
        </el-button>
        <el-button :icon="Refresh" @click="refreshData">
          刷新
        </el-button>
      </div>
    </div>

    <el-row :gutter="20" class="stats-cards">
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stats-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><DataLine /></el-icon>
              <span>总记录数</span>
            </div>
          </template>
          <div class="stats-value">
            <span class="value">{{ pagination.total }}</span>
            <span class="unit">条</span>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stats-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Sunny /></el-icon>
              <span>平均温度</span>
            </div>
          </template>
          <div class="stats-value">
            <span class="value">{{ Number.isFinite(stats.avgTemperature) ? stats.avgTemperature.toFixed(1) : '--' }}</span>
            <span class="unit">°C</span>
          </div>
          <div class="stats-info">
            <p>最高: {{ Number.isFinite(stats.maxTemperature) ? stats.maxTemperature.toFixed(1) : '--' }}°C</p>
            <p>最低: {{ Number.isFinite(stats.minTemperature) ? stats.minTemperature.toFixed(1) : '--' }}°C</p>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stats-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><HotWater /></el-icon>
              <span>平均湿度</span>
            </div>
          </template>
          <div class="stats-value">
            <span class="value">{{ Number.isFinite(stats.avgHumidity) ? stats.avgHumidity.toFixed(1) : '--' }}</span>
            <span class="unit">%</span>
          </div>
          <div class="stats-info">
            <p>最高: {{ Number.isFinite(stats.maxHumidity) ? stats.maxHumidity.toFixed(1) : '--' }}%</p>
            <p>最低: {{ Number.isFinite(stats.minHumidity) ? stats.minHumidity.toFixed(1) : '--' }}%</p>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stats-card">
          <template #header>
            <div class="card-header">
              <el-icon size="18"><Timer /></el-icon>
              <span>最近更新</span>
            </div>
          </template>
          <div class="stats-value">
            <span class="value-small">{{ lastUpdateTime || '--' }}</span>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <el-card shadow="hover" class="data-table-card">
      <template #header>
        <div class="card-header">
          <el-icon size="18"><Tickets /></el-icon>
          <span>历史记录</span>
          <div class="table-controls">
            <el-input
              v-model="searchKeyword"
              placeholder="搜索温度或湿度"
              :prefix-icon="Search"
              style="width: 200px; margin-right: 10px;"
              clearable
              @input="handleSearch"
            />
            <el-radio-group v-model="viewMode" size="small">
              <el-radio-button label="table">
                <el-icon><Grid /></el-icon> 表格
              </el-radio-button>
              <el-radio-button label="chart">
                <el-icon><Histogram /></el-icon> 图表
              </el-radio-button>
            </el-radio-group>
          </div>
        </div>
      </template>

      <div v-if="viewMode === 'table'">
        <el-table
          :data="tableData"
          :loading="loading"
          height="500"
          stripe
          border
          style="width: 100%;"
        >
          <el-table-column type="index" label="序号" width="70" align="center" />
          
          <el-table-column prop="recorded_at" label="时间" width="180" sortable>
            <template #default="{ row }">
              {{ formatDateTime(row.recorded_at) }}
            </template>
          </el-table-column>
          
          <el-table-column prop="device_id" label="设备ID" width="120" />
          
          <el-table-column prop="temperature" label="温度 (°C)" width="130" sortable>
            <template #default="{ row }">
              <span :class="getTemperatureClass(row.temperature)">
                {{ parseFloat(row.temperature).toFixed(1) }}
              </span>
            </template>
          </el-table-column>
          
          <el-table-column prop="humidity" label="湿度 (%)" width="130" sortable>
            <template #default="{ row }">
              <span :class="getHumidityClass(row.humidity)">
                {{ parseFloat(row.humidity).toFixed(1) }}
              </span>
            </template>
          </el-table-column>
          
          <el-table-column prop="pressure" label="气压 (hPa)" width="130">
            <template #default="{ row }">
              {{ parseFloat(row.pressure).toFixed(1) }}
            </template>
          </el-table-column>
          
          <el-table-column prop="gas" label="气体电阻 (Ω)" width="150">
            <template #default="{ row }">
              {{ parseFloat(row.gas).toFixed(0) }}
            </template>
          </el-table-column>
          
          <el-table-column prop="co2" label="二氧化碳 (PPM)" width="140">
            <template #default="{ row }">
              {{ row.co2 !== null ? parseFloat(row.co2).toFixed(0) : '--' }}
            </template>
          </el-table-column>
          
          <el-table-column prop="uptime" label="运行时间 (秒)" width="140">
            <template #default="{ row }">
              {{ row.uptime || '--' }}
            </template>
          </el-table-column>
          
          <el-table-column label="操作" width="100" fixed="right">
            <template #default="{ row }">
              <el-button size="small" @click="showDetail(row)">
                详情
              </el-button>
            </template>
          </el-table-column>
        </el-table>

        <div class="pagination-container">
          <el-pagination
            v-model:current-page="pagination.page"
            v-model:page-size="pagination.limit"
            :page-sizes="[10, 20, 50, 100]"
            :total="pagination.total"
            layout="total, sizes, prev, pager, next, jumper"
            @size-change="handleSizeChange"
            @current-change="handleCurrentChange"
          />
        </div>
      </div>

      <div v-else class="chart-view">
        <div class="chart-controls">
          <el-radio-group v-model="chartType" size="small">
            <el-radio-button label="line">折线图</el-radio-button>
            <el-radio-button label="bar">柱状图</el-radio-button>
          </el-radio-group>
          
          <el-select v-model="chartMetric" size="small" style="width: 120px; margin-left: 10px;">
            <el-option label="温度" value="temperature" />
            <el-option label="湿度" value="humidity" />
            <el-option label="温度与湿度" value="both" />
            <el-option label="气压" value="pressure" />
            <el-option label="二氧化碳" value="co2" />
          </el-select>
          
          <el-button
            size="small"
            :icon="Download"
            @click="exportChart"
            style="margin-left: 10px;"
          >
            导出图表
          </el-button>
        </div>
        
        <div ref="historyChart" style="width: 100%; height: 400px;"></div>
      </div>
    </el-card>

    <el-dialog
      v-model="showDetailDialog"
      title="数据详情"
      width="600px"
    >
      <div v-if="selectedRow" class="detail-content">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="时间">
            {{ formatDateTime(selectedRow.recorded_at) }}
          </el-descriptions-item>
          <el-descriptions-item label="设备ID">
            {{ selectedRow.device_id }}
          </el-descriptions-item>
          <el-descriptions-item label="温度">
            <el-tag :type="getTemperatureTagType(selectedRow.temperature)" size="small">
              {{ parseFloat(selectedRow.temperature).toFixed(1) }}°C
            </el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="湿度">
            <el-tag :type="getHumidityTagType(selectedRow.humidity)" size="small">
              {{ parseFloat(selectedRow.humidity).toFixed(1) }}%
            </el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="气压">
            {{ parseFloat(selectedRow.pressure).toFixed(1) }} hPa
          </el-descriptions-item>
          <el-descriptions-item label="气体阻力">
            {{ parseFloat(selectedRow.gas).toFixed(0) }} Ω
          </el-descriptions-item>
          <el-descriptions-item label="二氧化碳">
            {{ selectedRow.co2 !== null ? parseFloat(selectedRow.co2).toFixed(0) + ' PPM' : '--' }}
          </el-descriptions-item>
          <el-descriptions-item label="运行时间">
            {{ selectedRow.uptime ? selectedRow.uptime + ' 秒' : '--' }}
          </el-descriptions-item>
          <el-descriptions-item label="记录ID">
            {{ selectedRow.id }}
          </el-descriptions-item>
        </el-descriptions>
        
        <div class="json-viewer">
          <h4>原始 JSON 数据:</h4>
          <pre>{{ formatJSON(selectedRow) }}</pre>
        </div>
      </div>
      
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="showDetailDialog = false">关闭</el-button>
          <el-button type="primary" @click="copyData">复制数据</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, watch, nextTick, onUnmounted } from 'vue'
import * as echarts from 'echarts'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Download,
  Refresh,
  Search,
  DataLine,
  Sunny,
  HotWater,
  Tickets,
  Grid,
  Histogram,
  Timer
} from '@element-plus/icons-vue'

const API_BASE_URL = 'http://1.94.161.42:3000'

const viewMode = ref('table')
const chartType = ref('line')
const chartMetric = ref('temperature')
const searchKeyword = ref('')
const loading = ref(false)
const showDetailDialog = ref(false)
const selectedRow = ref(null)
const historyChart = ref(null)
let chartInstance = null

const tableData = ref([])
const stats = ref({
  avgTemperature: null,
  maxTemperature: null,
  minTemperature: null,
  avgHumidity: null,
  maxHumidity: null,
  minHumidity: null
})

const pagination = ref({
  page: 1,
  limit: 20,
  total: 0,
  totalPages: 0
})

const lastUpdateTime = ref('')

const fetchHistoryData = async () => {
  loading.value = true
  try {
    const params = new URLSearchParams({
      page: pagination.value.page,
      limit: pagination.value.limit
    })
    
    const response = await fetch(`${API_BASE_URL}/api/history?${params}`)
    const result = await response.json()
    
    if (result.success) {
      tableData.value = result.data
      pagination.value.total = result.pagination.total
      pagination.value.totalPages = result.pagination.totalPages
      
      if (result.data.length > 0) {
        lastUpdateTime.value = formatDateTime(result.data[0].recorded_at)
      }
    }
  } catch (error) {
    console.error('获取历史数据失败:', error)
    ElMessage.error('获取历史数据失败')
  } finally {
    loading.value = false
  }
}

const fetchStats = async () => {
  try {
    const response = await fetch(`${API_BASE_URL}/api/stats`)
    const result = await response.json()
    
    if (result.success) {
      const data = result.data
      stats.value = {
        avgTemperature: parseFloat(data.avg_temperature),
        maxTemperature: parseFloat(data.max_temperature),
        minTemperature: parseFloat(data.min_temperature),
        avgHumidity: parseFloat(data.avg_humidity),
        maxHumidity: parseFloat(data.max_humidity),
        minHumidity: parseFloat(data.min_humidity)
      }
    }
  } catch (error) {
    console.error('获取统计数据失败:', error)
  }
}

const handleSearch = () => {
  pagination.value.page = 1
  fetchHistoryData()
}

const handleSizeChange = (size) => {
  pagination.value.limit = size
  pagination.value.page = 1
  fetchHistoryData()
}

const handleCurrentChange = (page) => {
  pagination.value.page = page
  fetchHistoryData()
}

const showDetail = (row) => {
  selectedRow.value = row
  showDetailDialog.value = true
}

const copyData = async () => {
  if (!selectedRow.value) return
  
  try {
    const text = JSON.stringify(selectedRow.value, null, 2)
    await navigator.clipboard.writeText(text)
    ElMessage.success('数据已复制到剪贴板')
  } catch (err) {
    console.error('复制失败:', err)
    ElMessage.error('复制失败')
  }
}

const formatDateTime = (timestamp) => {
  if (!timestamp) return '--'
  // 直接解析字符串，避免 new Date() 的时区转换偏差
  const str = String(timestamp)
  // 匹配 "2026-06-10 21:28:00" 或 "2026-06-10T21:28:00.000Z" 等格式
  const match = str.match(/^(\d{4})-(\d{2})-(\d{2})[T ](\d{2}):(\d{2}):(\d{2})/)
  if (match) {
    const [, year, month, day, hour, minute, second] = match
    return `${year}-${month}-${day} ${hour}:${minute}:${second}`
  }
  // 回退：如果是纯数字时间戳，仍用 Date
  const date = new Date(timestamp)
  if (!isNaN(date.getTime())) {
    return `${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')} ${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`
  }
  return '--'
}

const formatJSON = (data) => {
  try {
    return JSON.stringify(data, null, 2)
  } catch {
    return data
  }
}

const getTemperatureClass = (temp) => {
  const value = parseFloat(temp)
  if (value < 32) return 'temp-low'
  if (value > 38) return 'temp-high'
  return 'temp-normal'
}

const getHumidityClass = (hum) => {
  const value = parseFloat(hum)
  if (value < 50) return 'hum-low'
  if (value > 75) return 'hum-high'
  return 'hum-normal'
}

const getTemperatureTagType = (temp) => {
  const value = parseFloat(temp)
  if (value < 32) return 'info'
  if (value > 38) return 'danger'
  return 'success'
}

const getHumidityTagType = (hum) => {
  const value = parseFloat(hum)
  if (value < 50) return 'info'
  if (value > 75) return 'warning'
  return 'success'
}

const refreshData = () => {
  fetchHistoryData()
  fetchStats()
  ElMessage.success('数据已刷新')
}

const exportData = () => {
  const data = tableData.value.map(item => ({
    '时间': formatDateTime(item.recorded_at),
    '设备ID': item.device_id,
    '温度(°C)': parseFloat(item.temperature).toFixed(1),
    '湿度(%)': parseFloat(item.humidity).toFixed(1),
    '气压(hPa)': parseFloat(item.pressure).toFixed(1),
    '气体阻力(Ω)': parseFloat(item.gas).toFixed(0),
    '二氧化碳(PPM)': item.co2 !== null ? parseFloat(item.co2).toFixed(0) : '--',
    '运行时间(秒)': item.uptime || '--'
  }))
  
  const headers = Object.keys(data[0] || {})
  const csvContent = [headers.join(','), ...data.map(item => Object.values(item).join(','))].join('\n')
  
  const BOM = '\uFEFF'
  const blob = new Blob([BOM + csvContent], { type: 'text/csv;charset=utf-8' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `CoolBeeBox历史数据_${new Date().toISOString().slice(0, 10)}.csv`
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
  URL.revokeObjectURL(url)
  
  ElMessage.success('数据导出成功')
}

const exportChart = () => {
  if (!chartInstance) return
  
  const dataURL = chartInstance.getDataURL({
    type: 'png',
    pixelRatio: 2,
    backgroundColor: '#fff'
  })
  
  const link = document.createElement('a')
  link.href = dataURL
  link.download = `CoolBeeBox历史图表_${new Date().toISOString().slice(0, 10)}.png`
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
}

const initChart = async () => {
  if (!historyChart.value) return
  
  chartInstance = echarts.init(historyChart.value)
  
  try {
    const params = new URLSearchParams({
      page: 1,
      limit: 100
    })
    
    const response = await fetch(`${API_BASE_URL}/api/history?${params}`)
    const result = await response.json()
    
    if (!result.success || !result.data.length) {
      ElMessage.warning('暂无数据用于图表展示')
      return
    }
    
    const data = result.data.slice().reverse()
    
    const tempData = data.map(item => [new Date(item.recorded_at), parseFloat(item.temperature)])
    const humData = data.map(item => [new Date(item.recorded_at), parseFloat(item.humidity)])
    const pressureData = data.map(item => [new Date(item.recorded_at), parseFloat(item.pressure)])
    const co2Data = data.map(item => [new Date(item.recorded_at), item.co2 === null ? null : parseFloat(item.co2)])

    const metricConfig = {
      temperature: { name: '温度', unit: '°C', color: '#ff6b6b', data: tempData },
      humidity: { name: '湿度', unit: '%', color: '#4d96ff', data: humData },
      pressure: { name: '气压', unit: 'hPa', color: '#7967ad', data: pressureData },
      co2: { name: '二氧化碳', unit: 'PPM', color: '#d99520', data: co2Data }
    }
    const selectedMetrics = chartMetric.value === 'both'
      ? ['temperature', 'humidity']
      : [chartMetric.value]
    
    const option = {
      tooltip: {
        trigger: 'axis',
        formatter: function(params) {
          const date = new Date(params[0].value[0])
          const time = `${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')} ${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}`
          let result = `${time}<br/>`
          params.forEach(item => {
            const value = item.value[1]
            const config = Object.values(metricConfig).find(metric => metric.name === item.seriesName)
            const decimals = config?.unit === 'PPM' ? 0 : 1
            const displayValue = Number.isFinite(value) ? value.toFixed(decimals) : '--'
            result += `${item.marker} ${item.seriesName}: ${displayValue}${config?.unit || ''}<br/>`
          })
          return result
        }
      },
      legend: {
        data: selectedMetrics.map(metric => metricConfig[metric].name)
      },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        containLabel: true
      },
      xAxis: {
        type: 'time',
        boundaryGap: false
      },
      yAxis: selectedMetrics.map((metric, index) => ({
        type: 'value',
        name: `${metricConfig[metric].name} (${metricConfig[metric].unit})`,
        position: index === 0 ? 'left' : 'right',
        min: metric === 'co2' ? 0 : undefined
      })),
      series: []
    }

    selectedMetrics.forEach((metric, index) => {
      const config = metricConfig[metric]
      option.series.push({
        name: config.name,
        type: chartType.value,
        yAxisIndex: index,
        data: config.data,
        smooth: chartType.value === 'line',
        connectNulls: false,
        lineStyle: { width: 3, color: config.color },
        itemStyle: { color: config.color }
      })
    })
    
    chartInstance.setOption(option)
  } catch (error) {
    console.error('初始化图表失败:', error)
    ElMessage.error('初始化图表失败')
  }
}

watch(viewMode, (newMode) => {
  if (newMode === 'chart') {
    nextTick(() => {
      initChart()
    })
  }
})

watch([chartType, chartMetric], () => {
  if (viewMode.value === 'chart' && chartInstance) {
    initChart()
  }
})

const handleResize = () => {
  if (chartInstance) {
    chartInstance.resize()
  }
}

onMounted(() => {
  fetchHistoryData()
  fetchStats()
  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  if (chartInstance) {
    chartInstance.dispose()
  }
  window.removeEventListener('resize', handleResize)
})
</script>

<style scoped>
.history-container {
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

.stats-cards {
  margin-bottom: 20px;
}

.stats-card {
  height: 100%;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
}

.stats-value {
  display: flex;
  align-items: baseline;
  margin-bottom: 10px;
}

.stats-value .value {
  font-size: 24px;
  font-weight: bold;
  color: #1a1a1a;
}

.stats-value .value-small {
  font-size: 16px;
  font-weight: bold;
  color: #1a1a1a;
}

.stats-value .unit {
  font-size: 14px;
  color: #404040;
  margin-left: 5px;
}

.stats-info p {
  margin: 5px 0;
  font-size: 14px;
  color: #303030;
}

.data-table-card {
  margin-bottom: 20px;
}

.table-controls {
  display: flex;
  align-items: center;
}

.temp-low {
  color: #606060;
}

.temp-normal {
  color: #2d8f2d;
}

.temp-high {
  color: #d93636;
  font-weight: bold;
}

.hum-low {
  color: #606060;
}

.hum-normal {
  color: #1a1a1a;
}

.hum-high {
  color: #d97300;
  font-weight: bold;
}

.pagination-container {
  display: flex;
  justify-content: flex-end;
  margin-top: 20px;
}

.chart-view {
  padding: 10px 0;
}

.chart-controls {
  display: flex;
  align-items: center;
  margin-bottom: 15px;
}

.detail-content {
  padding: 10px 0;
}

.json-viewer {
  margin-top: 20px;
  padding: 15px;
  background-color: #f5f7fa;
  border-radius: 4px;
  border: 1px solid #ebeef5;
}

.json-viewer h4 {
  margin-top: 0;
  margin-bottom: 10px;
}

.json-viewer pre {
  margin: 0;
  white-space: pre-wrap;
  word-wrap: break-word;
  font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
  font-size: 12px;
}

@media screen and (max-width: 768px) {
  .history-container {
    padding: 12px;
  }
  
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
    margin-bottom: 16px;
  }
  
  .header-actions {
    width: 100%;
    flex-direction: column;
    gap: 8px;
  }
  
  .stats-cards,
  .data-table-card {
    margin-bottom: 16px;
  }
  
  .stats-value .value {
    font-size: 20px;
  }
  
  .stats-value .value-small {
    font-size: 14px;
  }
  
  .stats-info p {
    font-size: 12px;
  }
  
  .table-controls {
    width: 100%;
    flex-direction: column;
    gap: 10px;
    align-items: stretch;
  }

  .table-controls :deep(.el-input) {
    width: 100% !important;
    margin-right: 0 !important;
  }

  .table-controls :deep(.el-radio-group) {
    display: flex;
  }

  .table-controls :deep(.el-radio-button) {
    flex: 1;
  }

  .table-controls :deep(.el-radio-button__inner) {
    width: 100%;
  }
  
  .pagination-container {
    justify-content: center;
  }
  
  .chart-controls {
    flex-direction: column;
    align-items: flex-start;
    gap: 10px;
  }
  
  .json-viewer {
    padding: 10px;
  }
  
  .json-viewer pre {
    font-size: 11px;
  }
}
</style>
