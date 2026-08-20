<template>
  <div class="app-shell">
    <aside class="app-sidebar">
      <div class="brand-block">
        <div class="brand-mark"><Sunny /></div>
        <div>
          <strong>CoolBee</strong>
          <span>蜂箱环境监测</span>
        </div>
      </div>

      <el-menu
        class="sidebar-menu"
        :router="true"
        :default-active="activeMenuIndex"
      >
        <el-menu-item v-for="item in navigation" :key="item.path" :index="item.path">
          <el-icon><component :is="item.icon" /></el-icon>
          <span>{{ item.label }}</span>
        </el-menu-item>
      </el-menu>

      <div class="sidebar-footer">
        <span class="status-dot"></span>
        <div>
          <strong>监测服务运行中</strong>
          <span>CoolBee IoT v1.0</span>
        </div>
      </div>
    </aside>

    <section class="app-workspace">
      <header class="app-header">
        <div class="page-identity">
          <span class="page-kicker">控制中心</span>
          <h1>{{ currentPage.title }}</h1>
          <p>{{ currentPage.description }}</p>
        </div>
        <div class="header-meta">
          <div class="live-pill"><span></span> LIVE</div>
          <div class="time-block">
            <strong>{{ currentTime }}</strong>
            <span>{{ currentDate }}</span>
          </div>
        </div>
      </header>

      <main class="app-content">
        <router-view />
      </main>
    </section>

    <nav class="mobile-nav" aria-label="主导航">
      <router-link v-for="item in navigation" :key="item.path" :to="item.path">
        <el-icon><component :is="item.icon" /></el-icon>
        <span>{{ item.shortLabel }}</span>
      </router-link>
    </nav>
  </div>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { Cpu, DataLine, Monitor, Sunny, Tickets } from '@element-plus/icons-vue'

const route = useRoute()
const currentTime = ref('')
const currentDate = ref('')
let timer

const navigation = [
  { path: '/', label: '总览面板', shortLabel: '总览', icon: Monitor },
  { path: '/realtime', label: '实时监测', shortLabel: '实时', icon: DataLine },
  { path: '/history', label: '历史数据', shortLabel: '历史', icon: Tickets },
  { path: '/ai', label: 'AI 分析', shortLabel: 'AI', icon: Cpu }
]

const pageContent = {
  '/': { title: '蜂箱总览', description: '关键环境指标与设备状态' },
  '/realtime': { title: '实时监测', description: '传感器数据流与设备控制' },
  '/history': { title: '历史数据', description: '查看、筛选并导出监测记录' },
  '/ai': { title: 'AI 分析', description: '智能诊断蜂箱环境与异常趋势' }
}

const activeMenuIndex = computed(() => route.path)
const currentPage = computed(() => pageContent[route.path] || pageContent['/'])

const updateClock = () => {
  const now = new Date()
  currentTime.value = now.toLocaleTimeString('zh-CN', {
    hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false
  })
  currentDate.value = now.toLocaleDateString('zh-CN', {
    month: 'long', day: 'numeric', weekday: 'short'
  })
}

onMounted(() => {
  updateClock()
  timer = window.setInterval(updateClock, 1000)
})

onUnmounted(() => window.clearInterval(timer))
</script>

<style scoped>
.app-shell { min-height: 100dvh; background: var(--bg-color); }

.app-sidebar {
  position: fixed;
  inset: 0 auto 0 0;
  z-index: 20;
  display: flex;
  width: 232px;
  flex-direction: column;
  padding: 24px 16px 18px;
  color: #fff;
  background: #182722;
  border-right: 1px solid rgba(255, 255, 255, 0.08);
}

.brand-block { display: flex; align-items: center; gap: 12px; padding: 0 8px 28px; }
.brand-mark {
  display: grid;
  width: 42px;
  height: 42px;
  flex: 0 0 42px;
  place-items: center;
  color: #182722;
  background: #f3b638;
  border-radius: 8px;
}
.brand-mark :deep(svg) { width: 24px; height: 24px; }
.brand-block strong { display: block; font-size: 20px; line-height: 1.1; }
.brand-block span { display: block; margin-top: 5px; color: #9bb0a8; font-size: 12px; }

.sidebar-menu { flex: 1; border: 0; background: transparent; }
.sidebar-menu :deep(.el-menu-item) {
  height: 46px;
  margin: 4px 0;
  color: #b9c8c2;
  border-radius: 7px;
}
.sidebar-menu :deep(.el-menu-item:hover) { color: #fff; background: #243a33; }
.sidebar-menu :deep(.el-menu-item.is-active) { color: #fff; background: #2e5f4f; }
.sidebar-menu :deep(.el-menu-item.is-active::before) {
  position: absolute;
  left: 0;
  width: 3px;
  height: 20px;
  content: '';
  background: #f3b638;
  border-radius: 0 3px 3px 0;
}

.sidebar-footer { display: flex; gap: 10px; align-items: flex-start; padding: 16px 10px 0; border-top: 1px solid #2b3e37; }
.sidebar-footer strong, .sidebar-footer span { display: block; }
.sidebar-footer strong { font-size: 12px; font-weight: 600; }
.sidebar-footer div > span { margin-top: 4px; color: #81958e; font-size: 11px; }
.status-dot { width: 8px; height: 8px; margin-top: 3px; background: #4bc48a; border-radius: 50%; box-shadow: 0 0 0 4px rgba(75, 196, 138, 0.13); }

.app-workspace { min-height: 100dvh; margin-left: 232px; }
.app-header {
  position: sticky;
  top: 0;
  z-index: 12;
  display: flex;
  min-height: 104px;
  align-items: center;
  justify-content: space-between;
  gap: 24px;
  padding: 20px clamp(24px, 3vw, 44px);
  background: rgba(247, 249, 248, 0.95);
  border-bottom: 1px solid var(--border-color);
  backdrop-filter: blur(12px);
}
.page-kicker { color: var(--primary-color); font-size: 11px; font-weight: 700; text-transform: uppercase; }
.page-identity h1 { margin: 3px 0 2px; color: var(--text-primary); font-size: 24px; line-height: 1.2; }
.page-identity p { margin: 0; color: var(--text-secondary); font-size: 13px; }
.header-meta { display: flex; align-items: center; gap: 18px; }
.live-pill { display: flex; align-items: center; gap: 7px; color: #357e64; font-size: 11px; font-weight: 700; }
.live-pill span { width: 7px; height: 7px; background: #3ab17e; border-radius: 50%; box-shadow: 0 0 0 4px rgba(58, 177, 126, 0.12); }
.time-block { min-width: 88px; padding-left: 18px; text-align: right; border-left: 1px solid var(--border-color); }
.time-block strong, .time-block span { display: block; }
.time-block strong { color: var(--text-primary); font-size: 15px; font-variant-numeric: tabular-nums; }
.time-block span { margin-top: 3px; color: var(--text-secondary); font-size: 11px; }
.app-content { padding: 0 clamp(24px, 3vw, 44px) 40px; overflow: hidden; }
.app-content :deep(.dashboard-container),
.app-content :deep(.realtime-container),
.app-content :deep(.history-container),
.app-content :deep(.ai-container) { padding: 0; }
.app-content :deep(.dashboard-container) { padding-top: 24px; }
.app-content :deep(.page-header) { justify-content: flex-end; }
.mobile-nav { display: none; }

@media (max-width: 768px) {
  .app-sidebar { display: none; }
  .app-workspace { margin-left: 0; padding-bottom: calc(72px + env(safe-area-inset-bottom)); }
  .app-header { min-height: 88px; padding: 14px 16px; }
  .page-identity h1 { font-size: 20px; }
  .page-identity p { max-width: 210px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .header-meta { gap: 0; }
  .live-pill, .time-block span { display: none; }
  .time-block { min-width: auto; padding-left: 12px; border-left: 0; }
  .time-block strong { font-size: 13px; }
  .app-content { padding: 0 12px 20px; }
  .app-content :deep(.dashboard-container) { padding-top: 16px; }
  .app-content :deep(.page-header) { align-items: stretch; }
  .mobile-nav {
    position: fixed;
    inset: auto 0 0;
    z-index: 50;
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    height: calc(64px + env(safe-area-inset-bottom));
    padding: 6px 8px env(safe-area-inset-bottom);
    background: rgba(255, 255, 255, 0.97);
    border-top: 1px solid var(--border-color);
    box-shadow: 0 -8px 24px rgba(27, 47, 40, 0.08);
    backdrop-filter: blur(14px);
  }
  .mobile-nav a { display: flex; min-width: 0; flex-direction: column; align-items: center; justify-content: center; gap: 3px; color: #7e8b86; font-size: 10px; text-decoration: none; }
  .mobile-nav .el-icon { width: 22px; height: 22px; font-size: 21px; }
  .mobile-nav a.router-link-active { color: var(--primary-color); font-weight: 700; }
}
</style>
