<template>
  <md-toolbar class="top-bar-desktop md-primary" style="position: fixed;">
    <h3 class="md-title" id="top-bar-title">
      <img class="top-bar-logo" :src="imgSrc" />
      {{$t(title)}}
    </h3>
    <md-menu md-direction="bottom-start">
      <md-button md-menu-trigger class="md-icon-button">
        <md-icon>translate</md-icon>
      </md-button>

      <md-menu-content>
        <md-menu-item @click="setLocale('zh')">中文</md-menu-item>
        <md-menu-item @click="setLocale('en')">English</md-menu-item>
      </md-menu-content>
    </md-menu>
  </md-toolbar>
</template>

<script>
const imgSrc = require('@/assets/wayz-logo.png');

console.log(imgSrc);
export default {
  name: 'TopBarDesktop',
  props: {
    title: String,
  },
  data() {
    return {
      imgSrc,
    };
  },
  methods: {
    setLocale(lang) {
      this.$i18n.locale = lang;
    },
  },
};
</script>

<style lang="scss" scoped>
@import "~vue-material/src/components/MdLayout/mixins";

.top-bar-desktop {
  @include md-layout-xsmall {
    display: none !important;
  }
  position: fixed;
  top: 0;
  left: 0;
}

.top-bar-logo {
  height: 32px;
}

#top-bar-title {
  flex: 1;
}
</style>
