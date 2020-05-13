import Vue from 'vue';
import VueRouter from 'vue-router';

import ViewHome from '@/views/Home.vue';
import ViewProfile from '@/views/Profile.vue';
import ViewProfileEdit from '@/views/ProfileEdit.vue';
import ViewMonitor from '@/views/Monitor.vue';
import ViewStorage from '@/views/Storage.vue';

Vue.use(VueRouter);

const routes = [
  {
    path: '',
    name: '主页',
    component: ViewHome
  },
  {
    path: '/profile',
    name: '传感器配置',
    component: ViewProfile
  },
  {
    path: '/profile/edit',
    name: '配置修改',
    component: ViewProfileEdit
  },
  {
    path: '/monitor',
    name: '数据监视',
    component: ViewMonitor
  },
  {
    path: '/storage',
    name: '数据管理',
    component: ViewStorage
  }
];

const router = new VueRouter({
  routes
});

export default router;
