<template lang="pug">
div
  van-cell-group(title="采集控制")
    van-cell(
      title="采集开关"
      center
    )
      van-switch(
        slot="right-icon"
        :value="started"
        @input="onClickSwitchStart"
        :disabled="!isFetched"
        :loading="isSwitchStartLoading"
        size="24px"
      )
    van-cell(
      title="录制数据"
      center
    )
      van-switch(
        slot="right-icon"
        :value="recording"
        @input="onClickSwitchRecord"
        :disabled="!started"
        :loading="isSwitchRecordLoading"
        size="24px"
      )

  van-dialog(
    title="操作错误"
    :value="isShowError"
    :message="errorReason"
    @confirm="clearError"
    theme="round-button"
  )

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';

const AcquisitionControlModule = namespace('AcquisitionControl');

@Component({})
export default class AcquisitionControl extends Vue {
  @AcquisitionControlModule.State isFetched;
  @AcquisitionControlModule.State fetchedData;

  @AcquisitionControlModule.Getter started;
  @AcquisitionControlModule.Getter recording;

  @AcquisitionControlModule.Getter isShowError;
  @AcquisitionControlModule.Getter errorReason;

  @AcquisitionControlModule.Action fetch;
  @AcquisitionControlModule.Action start;
  @AcquisitionControlModule.Action stop;
  @AcquisitionControlModule.Action setRecord;

  @AcquisitionControlModule.Action clearError;

  async onClickSwitchStart(value: boolean) {
    this.isSwitchStartLoading = true;
    if (value) {
      await this.start();
    } else {
      await this.stop();
    }
    this.isSwitchStartLoading = false;
  }

  async onClickSwitchRecord(value: boolean) {
    this.isSwitchRecordLoading = true;
    await this.setRecord(value);
    this.isSwitchRecordLoading = false;
  }

  isSwitchStartLoading = false;

  isSwitchRecordLoading = false;
}
</script>
