import Vue from 'vue';
import VueRouter from 'vue-router';

import ViewHome from '@/views/Home.vue';
import ViewProfile from '@/views/Profile.vue';
import ViewProfileEdit from '@/views/ProfileEdit.vue';
import ViewMonitor from '@/views/Monitor.vue';

Vue.use(VueRouter);

const routes = [
  {
    path: '',
    name: 'home',
    component: ViewHome
  },
  {
    path: '/profile',
    name: 'profile',
    component: ViewProfile
  },
  {
    path: '/profile/edit',
    name: 'profile/edit',
    component: ViewProfileEdit
  },
  {
    path: '/monitor',
    name: 'monitor',
    component: ViewMonitor
  }
];

const router = new VueRouter({
  routes
});

export default router;
