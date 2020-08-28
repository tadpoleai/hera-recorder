<template lang="pug">
.monitor(
  v-resize:throttle="onResize"
)
  van-cell-group(title="采集信息")
    van-cell
      div(style="display: flex; justify-content: space-between;")
        span 采集时间
        span {{timeFromStart}}

    DiskUsage(
      :refresh="true"
    )

    van-cell(
      title="数据刷新频率"
    )
      van-rate(
        slot="right-icon"
        v-model="dataRefreshRate"
        :size="24"
        :gutter="6"
      )

  van-cell-group(title="传感器数据")
    van-grid(
      :column-num="columnNum"
    )
      van-grid-item.device-grid-item(
        v-for="(deviceData, deviceIndex) in fetchedData.deviceDatas"
      )
        DetailData
        
        van-panel.device-panel(
          @click="setSelectDetailDeviceIndex(deviceIndex)"
        )
          van-cell.device-panel-header(
            slot="header"
          )
            div
              span {{deviceData.type + '/' + deviceData.name}}
            div(style="display: flex; justify-content: space-between;")
              span(style="font-size: 75%;")
                | {{frequencyFormat(deviceData.frequency)}}
              span(style="font-size: 75%;")
                | {{dataSizeFormatShort(deviceData.dataSizeKB)}}

          //- dispData
          template(
            v-for="singleDisplayData in deviceData.dispData"
          )
            JpegImage.device-data-image(
              v-if="singleDisplayData.jpegData.length != 0"
              :jpeg="singleDisplayData.jpegData"
            )
            template(
              v-if="singleDisplayData.textData.length != 0"
            )
              p.pdata(v-for="line in renderStringData(singleDisplayData.textData)") {{line}}

  //- van-cell-group(
  //-   v-if="status.local.operatorInfo.slam"
  //-   title="实时建图"
  //- )
  //-   img.device-data-image(
  //-     v-if="status.slamResultValid"
  //-     :ref="'imageslam'"
  //-   )
  //-   van-empty(
  //-     v-else
  //-     image="error"
  //-     description="尚无建图结果"
  //-   )
  
  //- van-overlay(
  //-   v-if="dataDetailIndex >= 0"
  //-   :show="showDataDetail"
  //-   @click="onClickData(-1)"
  //- )
  //-   .data-detail-wrapper
  //-     .data-detail
  //-       van-panel.device-panel
  //-         van-cell(slot="header")
  //-           div()
  //-             span {{status.deviceDatas[dataDetailIndex].type + '/' + status.deviceDatas[dataDetailIndex].name}}
  //-           div(style="display: flex; justify-content: space-between;")
  //-             span(style="font-size: 90%;") {{status.deviceDatas[dataDetailIndex].sequence}}
  //-             span(style="font-size: 90%;") {{renderFrequency(status.deviceDatas[dataDetailIndex].frequency)}}
  //-             span(style="font-size: 90%;") {{(status.deviceDatas[dataDetailIndex].dataSizeKB / 1024).toFixed(1) + 'M'}}

  //-         img.device-data-image(
  //-           v-if="status.deviceDatas[dataDetailIndex].isJpeg"
  //-           :ref="'imageDetail'")
  //-         template(v-else)
  //-           p.pdata(
  //-             v-for="line in renderStringData(status.deviceDatas[dataDetailIndex].data)"
  //-           ) {{line}}

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';
import { Dialog } from 'vant';

import { durationFormat, frequencyFormat, dataSizeFormatShort } from '@/utils';
import resize from 'vue-resize-directive';

import DetailData from '@/views/Monitor/DetailData.vue';

import Parameter from '@/components/Parameter.vue';
import DiskUsage from '@/components/DiskUsage.vue';
import JpegImage from '@/components/JpegImage.vue';

const DeviceDataModule = namespace('DeviceData');

@Component({
  components: { DetailData, DiskUsage, JpegImage, Parameter },
  directives: { resize }
})
export default class Monitor extends Vue {
  @DeviceDataModule.State fetchedData;

  @DeviceDataModule.Action fetch;

  @DeviceDataModule.Action setSelectDetailDeviceIndex;
  @DeviceDataModule.Action clearSelectDetailDeviceIndex;

  async created() {
    this.clearSelectDetailDeviceIndex();
    await this.fetch();
    this.timeoutHandler = setTimeout(this.timeoutFunction, this.intervalPeriod);
  }

  destroyed() {
    clearTimeout(this.timeoutHandler);
  }

  async timeoutFunction() {
    await this.fetch();
    this.timeoutHandler = setTimeout(this.timeoutFunction, this.intervalPeriod);
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

  renderStringData(rawData: Buffer) {
    return String(rawData).split('\n');
  }

  frequencyFormat = frequencyFormat;

  dataSizeFormatShort = dataSizeFormatShort;

  get timeFromStart() {
    if (this.fetchedData.startTimeSec == 0) {
      return 'N/A';
    }
    const duration = this.fetchedData.nowTimeSec - this.fetchedData.startTimeSec;
    return durationFormat(duration);
  }

  get intervalPeriod() {
    const RatePeriodTable = [4800, 2400, 1200, 600, 300, 150];
    let intRate = Math.floor(this.dataRefreshRate);
    if (intRate < 0) {
      intRate = 0;
    } else if (intRate > 5) {
      intRate = 5;
    }
    return RatePeriodTable[intRate];
  }

  columnNum = 2;

  dataRefreshRate = 1;

  timeoutHandler!: NodeJS.Timeout;
}
</script>

<style lang="stylus">
.van-grid-item__content
  padding 0 !important

.device-panel
  height 100%
  width 100%

.device-panel-header
  padding-left 8 !important
  padding-right 8 !important

.pdata
  padding 0
  margin 0.1rem 1rem

.device-data-image
  width 100%
</style>
