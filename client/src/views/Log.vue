<template lang="pug">
div
  van-cell-group(title='当前日志')
    virtual-list(
      style='height: calc(95vh - 96px); overflow-y: auto',
      :data-key='"index"',
      :data-sources='messages',
      :data-component='ListItem'
    )
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';

import ListItem from './Log/ListItem.vue';
import VirtualList from 'vue-virtual-scroll-list';

const LogModule = namespace('Log');

@Component({
  components: { ListItem, VirtualList }
})
export default class Storage extends Vue {
  ListItem = ListItem;

  @LogModule.State messages;
}
</script>

<style lang="stylus"></style>
