<template>
  <md-content class="navi-desktop">
    <md-list>
      <template v-for="entry in entries">
        <!-- single entry -->
        <md-list-item v-if="!entry.children" :key="entry.to" :to="entry.to">
          <md-icon :md-src='require("@/assets/icons/" + entry.icon + ".svg")'></md-icon>
          <span class="md-list-item-text">{{$t(entry.name)}}</span>
        </md-list-item>

        <!-- entry with children -->
        <md-list-item v-else :key="entry.to" md-expand :md-expanded="true">
          <md-icon :md-src='require("@/assets/icons/" + entry.icon + ".svg")'></md-icon>
          <span class="md-list-item-text">{{$t(entry.name)}}</span>

          <md-list slot="md-expand">
            <md-list-item
              v-for="child in entry.children"
              :key="child.to"
              :to="entry.to + '#' + child.to"
              class="md-inset"
            ><md-icon :md-src='require("@/assets/icons/" + child.icon + ".svg")'></md-icon>
            <span class="md-list-item-text">{{$t(child.name)}}</span>
            </md-list-item>
          </md-list>
        </md-list-item>
      </template>
    </md-list>
  </md-content>
</template>

<script>
export default {
  name: 'navi-desktop',
  props: { entries: Array },
};
</script>

<style lang="scss" scoped>
@import "~vue-material/src/components/MdLayout/mixins";
.navi-desktop {
  @include md-layout-xsmall {
    display: none;
  }
  position: fixed;
  top: 0;
  left: 0;
  width: 256px;
  display: flex;
  align-items: flex-start;
  flex-direction: column;
  overflow: auto;
  padding: 64px 0px 0px 0px;
  @include md-layout-small {
    padding: 48px 0px 0px 0px;
  }
  .md-list-item {
    width: 256px;
  }
}
</style>
