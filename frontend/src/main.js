import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'         // 引入 Element Plus
import 'element-plus/dist/index.css'           // 引入配套样式
import zhCn from 'element-plus/es/locale/lang/zh-cn'
import './assets/main.css'

import App from './App.vue'
import router from './router'
import { useDashboardStore } from './stores/dashboard'

const app = createApp(App)

app.use(createPinia())
app.use(router)
app.use(ElementPlus, { locale: zhCn })         // 全局使用

// 初始化Dashboard Store
const dashboardStore = useDashboardStore()
dashboardStore.initialize()

app.mount('#app')
