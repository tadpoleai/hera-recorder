import Vue, { VueConstructor } from 'vue';
import Router from 'vue-router';
import VAcquisition from '@/views/Acquisition.vue';
import VMonitor from '@/views/Monitor.vue';
import VStorage from '@/views/Storage.vue';

Vue.use(Router);

export interface RouteEntry {
  path: string,
  name: string,
  icon: string,
  component: Object
}

const routeList: Array<RouteEntry> = [
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
