<template lang="pug">
.upload
  van-panel(
  )
    div(
      slot="header"
      @click="onClickPanel()"
    )
      van-cell(
        :title="process.request.name"
        :label="process.running ? process.eta : ''"
        :border="false"
        center
      )
        span {{process.running ? process.speed: ''}}
        template(slot="right-icon")
          van-icon(
            v-if="process.errored"
            color="red" name="warning-o" size="large")
          van-icon(
            v-if="!process.running && !process.errored"
            color="green" name="passed" size="large")

      van-cell(
        :border="false"
        v-if="process.running || process.errored"
      )
        // Progress percentage
        van-progress(
          style="margin: 0.5em 0 1.5em 0"
          :percentage="percentage"
          :pivot-text="percentage.toFixed(0) + '%'"
          pivot-color="orange"
          color="orange"
        )

        .error-line(v-for="line in errorReasonLines") {{line}}

        div(style="display: flex; justify-content: space-around;")
          van-button(
            v-if="process.running"
            size="small"
            type="danger"
            @click="onClickAbort()"
          ) 终止
          van-button(
            v-if="process.errored"
            size="small"
            type="info"
            @click="onClickRetry()"
          ) 重试
          van-button(
            v-if="process.errored"
            size="small"
            type="danger"
            @click="onClickCancel()"
          ) 取消
</template>

<script lang="ts">
import { Component, Vue, Prop } from 'vue-property-decorator';
import { namespace } from 'vuex-class';
import { Hera } from '@/api';

const UploadModule = namespace('Upload');

@Component({})
export default class UploadProcess extends Vue {
  @Prop({ required: true })
  process!: Hera.UploadProcess;

  @UploadModule.Action completeUpload;
  @UploadModule.Action retryUpload;
  @UploadModule.Action abortUpload;

  get percentage() {
    if (!this.process.running && !this.process.errored) {
      return 100;
    }
    return (this.process.processedSize / this.process.totalSize) * 100;
  }

  get processStatus() {
    if (this.process.running) {
      return this.process.speed;
    }
    if (this.process.errored) {
      return '错误';
    }
    return '上传完成';
  }

  get errorReasonLines() {
    return this.process.reason.split(' \n ');
  }

  onClickPanel() {
    if (!this.process.running && !this.process.errored) {
      this.completeUpload(this.process.request);
    }
  }

  onClickRetry() {
    if (!this.process.running && this.process.errored) {
      this.retryUpload(this.process.request);
    }
  }

  onClickAbort() {
    if (this.process.running) {
      this.abortUpload(this.process.request);
    }
  }

  onClickCancel() {
    if (!this.process.running) {
      this.completeUpload(this.process.request);
    }
  }
}
</script>

<style lang="stylus" scoped>
.error-line
  color red
  font-size: 90%
  margin: 0 0 0.5em 0

.center
  text-align center

.right
  text-align right
</style>
