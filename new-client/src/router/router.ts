import Vue, { VueConstructor } from 'vue';
import Router from 'vue-router';
import Home from '@/views/Home.vue';
import About from '@/views/About.vue';

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
    name: 'home',
    icon: 'mdi-home',
    component: Home,
  },
  {
    path: '/about',
    name: 'about',
    icon: 'mdi-information-variant',
    component: About,
  },
];

const router = new Router({
  routes: routeList,
});

export default {
  routeList,
  router,
};
