<template>
  <md-bottom-bar class="navi-mobile" md-type="shift" md-sync-route>
    <md-bottom-bar-item
      v-for="entry in entries"
      :key="entry.name"
      :md-icon="require('@/assets/icons/' + entry.icon + '.svg')"
      :md-label="$t(entry.name)"
    ></md-bottom-bar-item>
  </md-bottom-bar>
</template>

<script>
export default {
  name: 'navi-mobile',
  props: { entries: Array },
};
</script>

<style lang="scss" scoped>
@import "~vue-material/src/components/MdLayout/mixins";
.navi-mobile {
  display: none;

  @include md-layout-xsmall {
    position: fixed;
    bottom: 0;
    z-index: 9;
    display: block;
  }
}
</style>
