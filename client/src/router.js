import Vue from 'vue';
import Router from 'vue-router';

Vue.use(Router);

export default new Router({
  // eslint-disable-next-line
  scrollBehavior(to, from, savedPosition) {
    if (to.hash) {
      return {
        selector: to.hash,
        offset: {
          x: 0,
          y: 64,
        },
      };
    }
    return {
      x: 0,
      y: 0,
    };
  },
  base: process.env.BASE_URL,
  routes: [
    {
      path: '/',
      redirect: '/home',
    },
    {
      path: '/home',
      name: 'Home',
      component: () => import('./views/Home.vue'),
    },
    {
      path: '/acquisition',
      name: 'Acquisition',
      component: () => import('./views/Acquisition.vue'),
    },
    {
      path: '/monitor',
      name: 'Monitor',
      component: () => import('./views/Monitor.vue'),
    },
    {
      path: '/storage',
      name: 'Storage',
      component: () => import('./views/Storage.vue'),
    },
  ],
});
