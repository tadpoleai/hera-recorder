<template>
  <md-toolbar id="toolbar-mobile" class="md-primary">
    <h3 id="toolbar-title" class="md-title">
      <img id="toolbar-logo" :src="wayzLogoImgSrc" />
      {{$t(title)}}
    </h3>
    <locale-setter />
  </md-toolbar>
</template>

<script>
import LocaleSetter from './LocaleSetter.vue';

const wayzLogoImgSrc = require('@/assets/wayz-logo.png');

export default {
  name: 'toolbar-mobile',
  props: {
    title: String,
  },
  components: {
    LocaleSetter,
  },
  data() {
    return {
      wayzLogoImgSrc,
    };
  },
};
</script>

<style lang="scss" scoped>
@import "~vue-material/src/components/MdLayout/mixins";

#toolbar-mobile {
  display: none !important;
  @include md-layout-xsmall {
    display: flex !important;
  }
}

#toolbar-logo {
  height: 32px;
}

#toolbar-title {
  flex: 1;
}
</style>
