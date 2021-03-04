<template lang="pug">
div
  van-sticky
    van-nav-bar(
      :left-arrow="$router.history.current.path != '/'"
      @click-left="onClickNavBack()"
      @click-right="onClickInfo()"
      :style="{ 'background': bgColor }"
    )
      template(slot="title")
        span {{'HERA ' + $router.history.current.name}}
      template(slot="right")
        van-icon(
          name="question-o"
          size="24px"
        )

  ConnectionError
  
  router-view

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';
import { Hera } from '@/api';
import { Toast, Dialog } from 'vant';
import { GitVersion } from '../git_info';

import ConnectionError from '@/views/App/ConnectionError.vue';

const MainModule = namespace('Main');
const MetaModule = namespace('Meta');
const AcquisitionControlModule = namespace('AcquisitionControl');

@Component({
  components: { ConnectionError }
})
export default class ProfileEdit extends Vue {
  // Actions
  @MainModule.Action refreshAll;

  @MainModule.Action refreshStatus;

  @MainModule.State isConnectionErrored;

  @AcquisitionControlModule.State fetchedData;

  @MetaModule.State daemonVersion;

  mounted() {
    this.active = true;
    (this.$router as any).history.current!.path !== '/' && this.$router.replace({ path: '/' });
    this.refreshAll();
    this.timeoutHandler = setTimeout(this.timeoutFunction, this.intervalPeriod);
  }

  active = false;

  intervalPeriod = 8000;

  timeoutHandler!: NodeJS.Timeout;

  destroyed() {
    this.active = false;
    clearTimeout(this.timeoutHandler);
  }

  async timeoutFunction() {
    if (!this.isConnectionErrored) {
      await this.refreshStatus();
    }
    if (this.active) {
      this.timeoutHandler = setTimeout(this.timeoutFunction, this.intervalPeriod);
    }
  }

  onClickNavBack() {
    if ((this.$router! as any).history!.current.path != '/') {
      this.$router.back();
    }
  }

  onClickInfo() {
    Dialog({
      title: 'HERA采集软件',
      message:
        '客户端版本\n' +
        GitVersion +
        '\n服务端版本\n' +
        this.daemonVersion +
        '\n版权信息\nCopyright 2018 Wayz.ai. All Rights Reserved.',
      messageAlign: 'left',
      theme: 'round-button'
    });
  }

  get bgColor() {
    if (this.isConnectionErrored) {
      return 'red';
    } else if (!this.fetchedData.started) {
      return '';
    } else if (this.fetchedData.recording) {
      return 'pink';
    } else {
      return 'lightblue';
    }
  }
}
</script>

<style lang="stylus">
html
  height: 100%
  background-color: #f7f8fa

#app
  font-family: Avenir, Helvetica, Arial, sans-serif
  -webkit-font-smoothing: antialiased
  -moz-osx-font-smoothing: grayscale
  text-align: center
  color: #2c3e50
  margin-top: 60px
</style>
