<template lang="pug">
.monitor
  van-nav-bar(title="HERA监视数据" left-arrow @click-left="clickNavBack()")

  van-cell-group(title="采集信息")
    van-cell
      div(style="display: flex; justify-content: space-between;")
        span 采集时间
        span {{timeFromStart}}

    van-cell
      div(style="display: flex; justify-content: space-between;")
        span 磁盘可用容量
        span {{diskVolume}}
      div
        van-progress(:percentage="diskVolumePercent"
        :show-pivot="false"
        stroke-width="8")

  van-cell-group(title="传感器数据")
    van-grid(column-num="2")
      van-grid-item.device-grid-item(
        v-for="(dd, index) in status.deviceDatas"
      )
        van-panel.device-panel
          van-cell(slot="header")
            div()
              span {{dd.type + '/' + dd.name}}
            div(style="display: flex; justify-content: space-between;")
              span(style="font-size: 90%;") {{dd.sequence}}
              span(style="font-size: 90%;") {{renderFrequency(dd.frequency)}}
              span(style="font-size: 90%;") {{(dd.dataSizeKB / 1024).toFixed(1) + 'M'}}

          img.device-data-image(v-if="dd.isJpeg" :ref="'image' + index")
          template(v-else)
            p.pdata(v-for="line in renderStringData(dd.data)") {{line}}

  van-cell-group(
    v-if="status.slamResultValid"
    title="实时建图")
    img.device-data-image(:ref="'imageslam'")
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
  }

  destroyed() {
    clearInterval(this.intervalHandler);
  }

  async updateData() {
    await Api.getData();
    for (let i = 0; i < this.status.deviceDatas.length; i += 1) {
      if (this.status.deviceDatas[i].isJpeg) {
        let img = new Image();
        img.src = 'data:image/jpg;base64,' + this.status.deviceDatas[i].data.toString('base64');
        img.onload = () => {
          if (this.$refs['image' + i] && (this.$refs['image' + i] as Element[])[0]) {
            ((this.$refs['image' + i] as Element[])[0] as any).src = img.src;
          }
        };
      }
    }

    if (this.status.slamResultValid) {
      const imgslam = new Image();
      imgslam.src = 'data:image/jpg;base64,' + this.status.slamResult.toString('base64');
      imgslam.onload = () => {
        if (this.$refs['imageslam']) {
          ((this.$refs['imageslam'] as Element) as any).src = imgslam.src;
        }
      };
    }
  }

  renderStringData(rawData: Buffer) {
    return String(rawData).split('\n');
  }

  renderJpegData(rawData: Buffer) {
    return 'data:image/jpg;base64,' + rawData.toString('base64');
  }

  renderFrequency(freq: number) {
    if (freq <= 1) {
      return freq.toFixed(2) + 'Hz';
    } else if (freq <= 10) {
      return freq.toFixed(1) + 'Hz';
    } else {
      return freq.toFixed(0) + 'Hz';
    }
  }

  get timeFromStart() {
    const totalSec = this.status.nowTimeSec - this.status.startTimeSec;
    const s = totalSec % 60;
    let ret = s + '秒 ';
    if (totalSec > 60) {
      const m = Math.floor(totalSec / 60) % 60;
      ret = m + '分 ' + ret;
    }
    if (totalSec > 3600) {
      const h = Math.floor(totalSec / 3600) % 24;
      ret = h + '小时 ' + ret;
    }
    if (totalSec > 86400) {
      const d = Math.floor(totalSec / 86400);
      ret = d + '天 ' + ret;
    }

    return ret;
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
  height 100%
  width 100%

.pdata
  padding 0
  margin 0.1rem 1rem

.device-data-image
  width 100%
</style>
