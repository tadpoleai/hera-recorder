<template>
  <div id="app" :style="{'font-family': fontFamily}">
    <TopBarDesktop :title="title"/>
    <NaviDesktop :entries="entries" />
  </div>
</template>

<script>
import TopBarDesktop from './components/TopBarDesktop.vue';
import NaviDesktop from './components/NaviDesktop.vue';

export default {
  name: 'app',
  components: {
    TopBarDesktop,
    NaviDesktop,
  },
  data() {
    return {
      title: 'Tron V5',
      entries: [
        { to: 'home', name: 'Home', icon: 'home' },
        {
          to: 'acquisition',
          name: 'Acquisition',
          icon: 'acquisition',
          children: [
            { to: 'control', name: 'Acquisition Control' },
            { to: 'profile', name: 'Acquisition Profile' },
          ],
        },
        { to: 'devices', name: 'Devices', icon: 'devices' },
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
    accent: md-get-palette-color(indigo, A200),
  )
);
@import "~vue-material/dist/theme/all"; // Apply the theme
</style>

<style lang="scss">
@import "~vue-material/src/components/MdLayout/mixins";

.view {
  padding: 64px 0 0 256px;
  @include md-layout-small {
    padding: 48px 0 0 256px;
  }
  @include md-layout-xsmall {
    padding: 0 0 64px 0;
  }
}
</style>
