<template lang="pug">
.monitor
  van-nav-bar(title="HERA监视数据" left-arrow @click-left="clickNavBack()")

  van-cell-group
    div(slot="title"
      style="display: flex; justify-content: space-between;")
        span 磁盘储存容量
        span {{diskVolume}}
    van-cell
      van-progress(:percentage="diskVolumePercent"
      :show-pivot="false"
      stroke-width="8")

  van-cell-group(title="传感器数据")
    van-grid(column-num="2")
      van-grid-item.device-grid-item(
        v-for="(dd, index) in status.deviceDatas"
      )
        van-panel.device-panel(
          :title="index + ' : ' + dd.type + '/' + dd.name"
          :status="' ' + dd.timeSecond"
          :desc="'sequence:' + dd.sequence + ' time:' + dd.timeSecond"
        )
          img.device-data-image(v-if="dd.isJpeg" :ref="'image' + index")
          template(v-else)
            p.pdata(v-for="line in renderStringData(dd.data)") {{line}}
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast } from 'vant';

@Component({})
export default class Home extends Vue {
  clickNavBack() {
    this.$router.back();
  }

  constructor() {
    super();
    this.updateData();
    this.intervalHandler = setInterval(this.updateData, Api.config.dataPeriodMs);
    console.log('Doki-Doki');
  }

  destroyed() {
    clearInterval(this.intervalHandler);
    console.log('Sayo-nara');
  }

  async updateData() {
    const deviceDatas = await Api.getData();
    for (let i = 0; i < this.status.deviceDatas.length; i += 1) {
      let img = new Image();
      img.src = 'data:image/jpg;base64,' + this.status.deviceDatas[i].data.toString('base64');
      img.onload = () => {
        if (this.$refs['image' + i] && (this.$refs['image' + i] as Element[])[0]) {
          ((this.$refs['image' + i] as Element[])[0] as any).src = img.src;
        }
      };
    }
    console.log('Just Monika');
  }

  renderStringData(rawData: Buffer) {
    return String(rawData).split('\n');
  }

  renderJpegData(rawData: Buffer) {
    return 'data:image/jpg;base64,' + rawData.toString('base64');
  }

  get diskVolumePercent() {
    if (!this.status.remoteStatus.diskTotalSpaceKB) {
      return 100;
    } else {
      return (this.status.remoteStatus.diskUsedSpaceKB / this.status.remoteStatus.diskTotalSpaceKB) * 100;
    }
  }

  get diskVolume() {
    return (
      (this.status.remoteStatus.diskUsedSpaceKB / 1024 / 1024).toFixed(3) +
      'GB / ' +
      (this.status.remoteStatus.diskTotalSpaceKB / 1024 / 1024).toFixed(3) +
      'GB'
    );
  }

  status = status;

  intervalHandler!: NodeJS.Timeout;
}
</script>

<style lang="stylus">
.van-grid-item__content
  padding 0 0 0 0 !important

.device-panel
  width 100%

.pdata
  padding 0
  margin 0.1rem 1rem

.device-data-image
  width 100%
</style>
