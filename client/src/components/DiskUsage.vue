<template lang="pug">
van-cell
  div(style="display: flex; justify-content: space-between;")
    span 已用容量
    template(v-if="isFetched")
      span {{diskUsedSpaceText}} / {{diskTotalSpaceText}}
    template(v-else)
      span 无数据
  van-progress(
    :percentage="percentage"
    :show-pivot="false"
    stroke-width="8"
    )
</template>

<script lang="ts">
import { Component, Prop, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';
import { dataSizeFormat } from '@/utils';

const DiskUsageModule = namespace('DiskUsage');

@Component({})
export default class DiskUsage extends Vue {
  @Prop({ required: true }) private refresh!: boolean;

  @DiskUsageModule.State isFetched;

  @DiskUsageModule.State fetchedData;

  @DiskUsageModule.Action fetch;

  created() {
    this.fetch();
  }

  get diskTotalSpaceText(): string {
    return dataSizeFormat(this.fetchedData.diskTotalSpaceKB);
  }

  get diskUsedSpaceText(): string {
    return dataSizeFormat(this.fetchedData.diskUsedSpaceKB);
  }

  get percentage(): number {
    if (this.fetchedData.diskTotalSpaceKB == 0) {
      return 0;
    } else {
      return (100.0 * this.fetchedData.diskUsedSpaceKB) / this.fetchedData.diskTotalSpaceKB;
    }
  }
}
</script>
