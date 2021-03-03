<template lang="pug">
div
  van-overlay(
    :show="show"
    @click="show = false"
  )

  transition(name="van-fade")
    div#fab-base.fab-container
      van-icon.fab-icon(
        size="16px"
        :name="show ? 'arrow-down' : 'arrow-up'"
        @click="show = !show"
      )

  transition-group(name="van-slide-up")
    div#fab-start.fab-container(
      v-show="show"
      key=1
      :style="{ background: !started ? 'white' : 'lightgrey' }"
    )
      van-icon.fab-icon(
        name="play"
        :color="!started ? 'blue' : 'grey' "
        size="24px"
        @click="onClickStart"
      )
      div.fab-text 启动采集

    div#fab-record.fab-container(
      v-show="show"
      key=2
      :style="{ background: started ? 'white' : 'lightgrey' }"
    )
      van-icon.fab-icon(
        :name="recording ? 'pause' : 'circle'"
        :color="started ? 'green' : 'grey' "
        size="20px"
        @click="onClickRecord"
      )
      div.fab-text {{ recording ? '暂停录制' : '开始录制' }}

    div#fab-stop.fab-container(
      v-show="show"
      key=3
      :style="{ background: started ? 'white' : 'lightgrey' }"
    )
      van-icon.fab-icon(
        name="stop"
        :color="started ? 'red' : 'grey' "
        size="32px"
        @click="onClickStop"
      )
      div.fab-text 停止采集

</template>

<script lang="ts">
import { Component, Vue, Watch } from 'vue-property-decorator';
import { namespace } from 'vuex-class';

const AcquisitionControlModule = namespace('AcquisitionControl');

@Component({})
export default class Fab extends Vue {
  @AcquisitionControlModule.Getter started;
  @AcquisitionControlModule.Getter recording;

  @AcquisitionControlModule.Action start;
  @AcquisitionControlModule.Action stop;
  @AcquisitionControlModule.Action setRecord;

  async onClickStart() {
    if (!this.started) {
      await this.start();
    }
    this.show = false;
  }

  async onClickRecord() {
    if (this.started) {
      await this.setRecord(!this.recording);
    }
    this.show = false;
  }

  async onClickStop() {
    if (this.started) {
      await this.stop();
    }
    this.show = false;
  }

  show = false;

  startLoading = false;
}
</script>

<style lang="stylus">
#fab-base
  bottom 24 * 2px !important;

#fab-start
  bottom 24 * 8px !important;

#fab-record
  bottom 24 * 6px !important;

#fab-stop
  bottom 24 * 4px !important;

.fab-container
  z-index 99 !important;
  position fixed !important;
  right 18px !important;
  width 36px !important;
  height 36px !important;
  line-height 36px !important;
  border-radius 18px !important;
  background white !important;

.fab-text
  position absolute !important;
  left -72px !important;
  top 6px !important;
  line-height 24px !important;
  width 64px !important;
  color lightgrey !important;

.fab-icon
  z-index 100 !important;
  top 0px !important;
  left 0px !important;
  width 36px !important;
  height 36px !important;
  line-height 36px !important;
  border-radius 18px !important;
  box-shadow 2px 2px 4px gray
  text-align center !important;
  cursor pointer !important;
.fab-icon:active
  box-shadow 0.5px 0.5px 1px gray !important;
</style>
