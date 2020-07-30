<template lang="pug">
.monitor(v-resize:throttle="onResize")
  van-cell-group(title="采集信息")
    van-cell
      div(style="display: flex; justify-content: space-between;")
        span 采集时间
        span {{timeFromStart}}

    van-cell
      div(style="display: flex; justify-content: space-between;")
        span 占用容量
        span {{diskVolume}}
      div
        van-progress(:percentage="diskVolumePercent"
        :show-pivot="false"
        stroke-width="8")

  van-cell-group(title="传感器数据")
    van-grid(:column-num="columnNum")
      van-grid-item.device-grid-item(
        v-for="(dd, index) in status.deviceDatas"
        @click="onClickData(index)"
      )
        van-panel.device-panel
          van-cell(slot="header")
            div()
              span {{dd.type + '/' + dd.name}}
            div(style="display: flex; justify-content: space-between;")
              span(style="font-size: 90%;") {{dd.sequence}}
              span(style="font-size: 90%;") {{renderFrequency(dd.frequency)}}
              span(style="font-size: 90%;") {{(dd.dataSizeKB / 1024).toFixed(1) + 'M'}}

          //- dispData
          template(v-for="(singleDisplayData, j) in dd.dispData")
              img.device-data-image(
                v-if="singleDisplayData.jpegData"
                :ref="'image' + index + ':' + j"
              )
              template(v-if="singleDisplayData.textData")
                p.pdata(v-for="line in renderStringData(singleDisplayData.textData)") {{line}}

  van-cell-group(
    v-if="status.local.operatorInfo.slam"
    title="实时建图"
  )
    img.device-data-image(
      v-if="status.slamResultValid"
      :ref="'imageslam'"
    )
    van-empty(
      v-else
      image="error"
      description="尚无建图结果"
    )
  
  van-overlay(
    v-if="dataDetailIndex >= 0"
    :show="showDataDetail"
    @click="onClickData(-1)"
  )
    .data-detail-wrapper
      .data-detail
        van-panel.device-panel
          van-cell(slot="header")
            div()
              span {{status.deviceDatas[dataDetailIndex].type + '/' + status.deviceDatas[dataDetailIndex].name}}
            div(style="display: flex; justify-content: space-between;")
              span(style="font-size: 90%;") {{status.deviceDatas[dataDetailIndex].sequence}}
              span(style="font-size: 90%;") {{renderFrequency(status.deviceDatas[dataDetailIndex].frequency)}}
              span(style="font-size: 90%;") {{(status.deviceDatas[dataDetailIndex].dataSizeKB / 1024).toFixed(1) + 'M'}}

          img.device-data-image(
            v-if="status.deviceDatas[dataDetailIndex].isJpeg"
            :ref="'imageDetail'")
          template(v-else)
            p.pdata(
              v-for="line in renderStringData(status.deviceDatas[dataDetailIndex].data)"
            ) {{line}}
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast } from 'vant';
import { durationFormat } from '@/utils';
import resize from 'vue-resize-directive';

@Component({
  directives: { resize: resize }
})
export default class Monitor extends Vue {
  constructor() {
    super();
    this.updateData();
    this.intervalHandler = setInterval(this.updateData, Api.config.dataPeriodMs);
  }

  destroyed() {
    clearInterval(this.intervalHandler);
  }

  onResize() {
    if (document.body.clientWidth < 600) {
      this.columnNum = 2;
    } else if (document.body.clientWidth < 1200) {
      this.columnNum = 3;
    } else if (document.body.clientWidth < 1600) {
      this.columnNum = 4;
    } else {
      this.columnNum = 6;
    }
  }

  onClickData(index: number) {
    this.dataDetailIndex = index;
  }

  async updateData() {
    await Api.getData();
    for (let i = 0; i < this.status.deviceDatas.length; i += 1) {
      const dispData = this.status.deviceDatas[i].dispData;
      for (let j = 0; j < dispData.length; j += 1) {
        if (dispData[j].jpegData !== undefined) {
          const img = new Image();
          img.src = 'data:image/jpg;base64,' + (dispData[j].jpegData as any).toString('base64');
          img.onload = () => {
            if (this.$refs['image' + i + ':' + j] && (this.$refs['image' + i + ':' + j] as Element[])[0]) {
              ((this.$refs['image' + i + ':' + j] as Element[])[0] as any).src = img.src;
            }
            if (i == this.dataDetailIndex) {
              if (this.$refs['imageDetail']) {
                ((this.$refs['imageDetail'] as Element) as any).src = img.src;
              }
            }
          };
        }
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
    const duration = this.status.nowTimeSec - this.status.startTimeSec;
    return durationFormat(duration);
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
      'GiB / ' +
      (this.status.remoteStatus.diskTotalSpaceKB / 1024 / 1024).toFixed(3) +
      'GiB'
    );
  }

  columnNum = 2;

  status = status;

  intervalHandler!: NodeJS.Timeout;

  showDataDetail = true;

  dataDetailIndex = -1;
}
</script>

<style lang="stylus" scoped>
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

.data-detail-wrapper
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;

.data-detail {
    width: 95vw;
    background-color: #fff;
  }
</style>
