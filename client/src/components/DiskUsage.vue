<template lang="pug">
van-cell
  div(style='display: flex; justify-content: space-between')
    span 已用容量
    template(v-if='isFetched')
      span {{ diskUsedSpaceText }} / {{ diskTotalSpaceText }}
    template(v-else)
      span 无数据
  van-progress(:percentage='percentage', :show-pivot='false', stroke-width='8')
</template>

<script lang="ts">
import { Component, Prop, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';
import { dataSizeFormat } from '@/utils';

const DiskUsageModule = namespace('DiskUsage');

@Component({})
export default class DiskUsage extends Vue {
  @DiskUsageModule.State isFetched;

  @DiskUsageModule.State fetchedData;

  @DiskUsageModule.Action fetch;

  get diskTotalSpaceText(): string {
    return dataSizeFormat(this.fetchedData.diskTotalSpace);
  }

  get diskUsedSpaceText(): string {
    return dataSizeFormat(this.fetchedData.diskUsedSpace);
  }

  get percentage(): number {
    if (this.fetchedData.diskTotalSpace == 0) {
      return 0;
    } else {
      return (100.0 * this.fetchedData.diskUsedSpace) / this.fetchedData.diskTotalSpace;
    }
  }
}
</script>
