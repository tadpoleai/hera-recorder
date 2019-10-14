import Vue, { VueConstructor } from 'vue';
import Router from 'vue-router';
import VHome from '@/views/Home.vue';
import VAcquisition from '@/views/Acquisition.vue';
import VMonitor from '@/views/Monitor.vue';
import VStorage from '@/views/About.vue';
import VAbout from '@/views/About.vue';

Vue.use(Router);

export interface RouteEntry {
  path: string,
  name: string,
  icon: string,
  component: Object
}

const routeList: Array<RouteEntry> = [
  {
    path: '/',
    name: 'Views.home',
    icon: 'mdi-home',
    component: VHome,
  },
  {
    path: '/acquisition',
    name: 'Views.acquisition',
    icon: 'mdi-camera-iris',
    component: VAcquisition,
  },
  {
    path: '/monitor',
    name: 'Views.monitor',
    icon: 'mdi-monitor-dashboard',
    component: VMonitor,
  },
  {
    path: '/storage',
    name: 'Views.storage',
    icon: 'mdi-folder-network',
    component: VStorage,
  },
];

const router = new Router({
  routes: routeList,
});

export default {
  routeList,
  router,
};
