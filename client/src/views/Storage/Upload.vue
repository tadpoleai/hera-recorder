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
            type="danger"
            @click="onClickRetry()"
          ) 重试
</template>

<script lang="ts">
import { Component, Vue, Prop } from 'vue-property-decorator';
import { Hera, Api } from '@/api';

@Component({})
export default class Upload extends Vue {
  @Prop({ required: true })
  process!: Hera.UploadProcess;

  get percentage() {
    if (!this.process.running && !this.process.errored) {
      return 100;
    }
    return (this.process.processedSizeKB / this.process.totalSizeKB) * 100;
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
      Api.completeUpload(this.process.request as Hera.UploadRequest);
    }
  }

  onClickRetry() {
    if (!this.process.running && this.process.errored) {
      Api.retryUpload(this.process.request as Hera.UploadRequest);
    }
  }

  onClickAbort() {
    if (this.process.running) {
      Api.abortUpload(this.process.request as Hera.UploadRequest);
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
