<template>
  <div id="app" :style="{'font-family': fontFamily}">
    <toolbar-desktop :title="title" />
    <navi-desktop :entries="entries" />
    <div id="view">
      <toolbar-mobile :title="title" />
      <keep-alive>
        <router-view id="router-view" />
      </keep-alive>
    </div>
    <navi-mobile :entries="entries" />
  </div>
</template>

<script>
import ToolbarDesktop from './components/ToolbarDesktop.vue';
import ToolbarMobile from './components/ToolbarMobile.vue';
import NaviDesktop from './components/NaviDesktop.vue';
import NaviMobile from './components/NaviMobile.vue';

export default {
  name: 'app',
  components: {
    ToolbarDesktop,
    ToolbarMobile,
    NaviDesktop,
    NaviMobile,
  },
  data() {
    return {
      title: 'Tron Platform',
      entries: [
        { to: 'home', name: 'Home', icon: 'home' },
        {
          to: 'acquisition',
          name: 'Acquisition',
          icon: 'device',
          children: [
            { to: 'control', name: 'Control', icon: 'power-setting' },
            { to: 'profile', name: 'Profile', icon: 'description' },
          ],
        },
        { to: 'monitor', name: 'Devices Monitoring', icon: 'pageview' },
        { to: 'storage', name: 'Storage', icon: 'storage' },
      ],
    };
  },
  computed: {
    fontFamily() {
      return {
        zh:
          "'Avenir', Verdana, 'Helvetica', Verdana, 'Hiragino Sans GB', 'Heiti SC', 'WenQuanYi Micro Hei', 'Microsoft Yahei', sans-serif",
        en: "'Avenir', Verdana, 'Helvetica', Verdana, Arial, sans-serif",
      };
    },
  },
};
</script>

<style lang="scss">
@import "~vue-material/dist/theme/engine"; // Import the theme engine
@include md-register-theme(
  "default",
  (
    primary: md-get-palette-color(indigo, 700),
    accent: md-get-palette-color(purple, A200)
  )
);
@import "~vue-material/dist/theme/all"; // Apply the theme

.md-card {
  margin: 4px;
  display: inline-block;
  vertical-align: top;
};
</style>

<style lang="scss" scoped>
@import "~vue-material/src/components/MdLayout/mixins";

#view {
  padding: 64px 0 0 256px;
  @include md-layout-small {
    padding: 48px 0 0 256px;
  }
  @include md-layout-xsmall {
    padding: 0 0 64px 0;
  }
}
</style>
