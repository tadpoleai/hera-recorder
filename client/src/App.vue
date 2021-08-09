<template lang="pug">
div(ref='app')
  van-nav-bar(:left-arrow='$router.history.current.path != "/"', @click-left='onClickNavBack()')
    template(slot='title')
      van-tag(v-show='tagName.show', :type='tagName.type') {{ tagName.msg }}
      span {{ "HERA " + $router.history.current.name }}
    template(slot='right')
      van-icon(name='notes-o', size='24px', @click='onClickLog()')
      van-icon(name='info-o', size='24px', @click='onClickInfo()')

  router-view(ref='view')

  Fab(v-if='useFab && !isErrorIgnored')

  ConnectionError
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';
import { Hera } from '@/api';
import { Toast, Dialog } from 'vant';
import { GitVersion } from '../git_info';

import ConnectionError from '@/views/App/ConnectionError.vue';
import Fab from '@/views/App/Fab.vue';

const MainModule = namespace('Main');
const MetaModule = namespace('Meta');
const PreferenceModule = namespace('Preference');
const AcquisitionControlModule = namespace('AcquisitionControl');
const LogModule = namespace('Log');

@Component({
  components: { ConnectionError, Fab }
})
export default class ProfileEdit extends Vue {
  @PreferenceModule.State useFab;

  // Actions
  @MainModule.Action refreshAll;

  @LogModule.Action syncLog;

  @MainModule.State isConnectionErrored;

  @MainModule.State isErrorIgnored;

  @MainModule.Mutation clearErrorIgnored;

  @AcquisitionControlModule.State fetchedData;

  @MetaModule.State daemonVersion;

  mounted() {
    this.active = true;
    (this.$router as any).history.current!.path !== '/' && this.$router.replace({ path: '/' });
    this.refreshAll();
    this.intervalHandler = setInterval(this.timeoutFunction, this.intervalPeriod);
  }

  active = false;

  intervalPeriod = 2500;

  intervalHandler!: NodeJS.Timeout;

  destroyed() {
    this.active = false;
    clearInterval(this.intervalHandler);
  }

  async timeoutFunction() {
    if (this.active) {
      if (!this.isConnectionErrored) {
        await this.syncLog();
      }
    }
  }

  async onClickNavBack() {
    if ((this.$refs['view'] as any).onClickNavBack) {
      const ret = await (this.$refs['view'] as any).onClickNavBack();
      if (!ret) {
        return;
      }
    }
    this.clearErrorIgnored();
    if ((this.$router! as any).history!.current.path != '/') {
      this.$router.back();
    }
  }

  onClickLog() {
    if ((this.$router! as any).history!.current.path != '/log') {
      this.$router.push('/log');
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

  get tagName(): { msg: string; type: string; show: boolean } {
    if (this.isConnectionErrored) {
      return { msg: '错误', type: 'danger', show: true };
    } else if (!this.fetchedData.started) {
      return { msg: '', type: '', show: false };
    } else if (this.fetchedData.recording) {
      return { msg: '保存中', type: 'warning', show: true };
    } else {
      return { msg: '未保存', type: 'primary', show: true };
    }
  }
}
</script>

<style lang="stylus">
html {
  height: 100%;
  background-color: #f7f8fa;
}

#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}
</style>
