import { createRouter, createWebHashHistory } from 'vue-router'
import DashboardView from '../views/DashboardView.vue'
import RealtimeView from '../views/RealtimeView.vue'
import HistoryView from '../views/HistoryView.vue'
import AiView from '../views/AiView.vue'

const router = createRouter({
  history: createWebHashHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      component: DashboardView
    },
    {
      path: '/realtime',
      name: 'realtime',
      component: RealtimeView
    },
    {
      path: '/history',
      name: 'history',
      component: HistoryView
    },
    {
      path: '/ai',
      name: 'ai',
      component: AiView
    }
  ]
})

export default router
