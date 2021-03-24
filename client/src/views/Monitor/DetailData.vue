<template lang="pug">
van-action-sheet(
  title="传感器数据"
  :value="isShowDetailDevice"
  @cancel="onCloseDetailDevice"
  @click-overlay="onCloseDetailDevice"
  safe-area-inset-bottom
)
  van-panel.device-panel
    van-cell.device-panel-header(
      slot="header"
    )
      div(style="display: flex; justify-content: space-between;")
        span {{deviceData.type + '/' + deviceData.name}}
        van-icon.title-right-icon.enabled-icon(
          name="edit"
          @click="isShowParameterEdit = !isShowParameterEdit"
          size="24px"
          v-bind:class="{ 'active-icon': isShowParameterEdit }"
        )
      div(style="display: flex; justify-content: space-between;")
        span {{frequencyFormat(deviceData.frequency)}}
        span {{dataSizeFormatShort(deviceData.dataSizeKB)}}

    //- dispData
    div.device-panel-content
      //- Edit Parameters
      template(
        v-if="isShowParameterEdit && isFetchedDeviceParameterses"
      )
        template(
          v-for="parameterRule in deviceRulesMap[deviceData.type]"
        )
          Parameter(
            :key="deviceData.type + '/' + deviceData.name + '/' + parameterRule.name"
            v-if="parameterRule.mutable"
            :rule="parameterRule"
            :valueString="currentParameters[parameterRule.name].toString()"
            isAsync
            @asyncInput="adjustDeviceParameter({type: parameterRule.name, value: $event})"
          )
      
      //- Show Detail Data
      template(
        v-for="singleDisplayData in deviceData.dispData"
      )
        JpegImage(
          v-if="singleDisplayData.jpegData.length != 0"
          :jpeg="singleDisplayData.jpegData"
        )
        template(
          v-if="singleDisplayData.textData.length != 0"
        )
          p.pdata(v-for="line in renderStringData(singleDisplayData.textData)") {{line}}

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';
import { Dialog } from 'vant';

import { frequencyFormat, dataSizeFormatShort } from '@/utils';

import Parameter from '@/components/Parameter.vue';
import JpegImage from '@/components/JpegImage.vue';

const DeviceDataModule = namespace('DeviceData');
const DeviceParameterModule = namespace('DeviceParameter');
const MetaModule = namespace('Meta');

@Component({
  components: { JpegImage, Parameter }
})
export default class DetailData extends Vue {
  @DeviceDataModule.State fetchedData;
  @DeviceDataModule.Getter('selectedDetailData') deviceData;

  @DeviceDataModule.State isShowDetailDevice;
  @DeviceDataModule.State selectDetailDeviceIndex;
  @DeviceDataModule.Action clearSelectDetailDeviceIndex;

  @DeviceParameterModule.State('isFetched') isFetchedDeviceParameterses;
  @DeviceParameterModule.Action('fetch') fetchDeviceParameterses;
  @DeviceParameterModule.State('fetchedData') deviceParameterses;
  @DeviceParameterModule.Action adjustDeviceParameter;

  @MetaModule.State deviceRulesMap;

  frequencyFormat = frequencyFormat;

  dataSizeFormatShort = dataSizeFormatShort;

  created() {
    this.fetchDeviceParameterses();
  }

  renderStringData(rawData: Buffer) {
    return String(rawData).split('\n');
  }

  onCloseDetailDevice() {
    this.clearSelectDetailDeviceIndex();
    this.isShowParameterEdit = false;
  }

  get currentParameters() {
    if (this.isFetchedDeviceParameterses) {
      return JSON.parse(this.deviceParameterses[this.selectDetailDeviceIndex].parametersJson);
    } else {
      return {};
    }
  }

  isShowParameterEdit = false;
}
</script>

<style lang="stylus">
.device-panel-header
  padding-left 8 !important
  padding-right 8 !important

.device-panel-content
  min-height 320px

.enabled-icon
  color #1989fa !important

.active-icon
  color red !important
</style>
