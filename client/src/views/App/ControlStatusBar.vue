<template lang="pug">
van-notice-bar(
  v-show="isShowStatusBar"
  :text="statusBarText"
  color="white"
  :background="statusBarBackground"
)
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';

const AcquisitionControlModule = namespace('AcquisitionControl');

@Component({})
export default class ControlStatusBar extends Vue {
  @AcquisitionControlModule.State fetchedData;

  get isShowStatusBar(): boolean {
    if (!this.fetchedData.started) {
      return false;
    } else {
      return this.fetchedData.started;
    }
  }

  get statusBarText(): string {
    if (!this.fetchedData.started) {
      return '';
    } else if (this.fetchedData.recording) {
      return '录制中: ' + this.fetchedData.storageFileName;
    } else {
      return '未录制: ' + this.fetchedData.storageFileName;
    }
  }

  get statusBarBackground(): string {
    if (!this.fetchedData.started) {
      return '';
    } else if (this.fetchedData.recording) {
      return 'pink';
    } else {
      return 'lime';
    }
  }
}
</script>
