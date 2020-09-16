import Vue from 'vue';
import VueRouter from 'vue-router';

import ViewHome from '@/views/Home.vue';
import ViewProfiles from '@/views/Profiles.vue';
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
    path: '/profiles',
    name: '配置',
    component: ViewProfiles
  },
  {
    path: '/profileedit',
    name: '修改配置',
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
