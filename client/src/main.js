import Vue from 'vue';
import VueI18n from 'vue-i18n';
import VueMaterial from 'vue-material';
import 'vue-material/dist/vue-material.min.css';
import router from './router';
import App from './App.vue';
import './registerServiceWorker';

const profileZh = require('./i18n/zh');

Vue.use(VueI18n);
const i18n = new VueI18n({
  locale: 'zh',
  messages: {
    zh: profileZh,
    en: {},
  },
  silentTranslationWarn: true,
});
Vue.use(VueMaterial);
Vue.config.productionTip = false;
new Vue({
  i18n,
  router,
  render: h => h(App),
}).$mount('#app');
