<template lang="pug">
div
  van-notice-bar(
    v-show="showNoticeBar"
    :text="noticeBarText"
    :background="noticeBarBackground"
    color="black"
  )

  router-view(v-if="status.remoteConnected")

  template(v-else)
    van-nav-bar(title="HERA采集软件")
    van-empty(image="network"
    :description="messageConnection")
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Api, status } from '@/api';
import { Toast } from 'vant';

@Component({})
export default class ProfileEdit extends Vue {
  created() {
    this.$router.replace({ path: '/' });
  }

  async mounted() {
    const result = await Api.get(true);
    if (result.error !== 0) {
      Toast.fail({
        message: Api.formatResult(result),
        duration: 0,
        closeOnClick: true
      });
      this.messageConnection = '连接下位机失败';
    }
    this.intervalHandler = setInterval(this.updateData, Api.config.syncPeriodMs);
  }

  updateData() {
    Api.get(false);
  }

  get showNoticeBar() {
    return this.status.remoteStatus.started;
  }

  get noticeBarText() {
    if (this.status.remoteStatus.recording) {
      return '保存中 > ' + this.status.remoteStatus.storageName;
    } else {
      return '未保存 > ' + this.status.remoteStatus.storageName;
    }
  }

  get noticeBarBackground() {
    if (this.status.remoteStatus.recording) {
      return 'lightpink';
    } else {
      return 'lightgreen';
    }
  }

  intervalHandler!: NodeJS.Timeout;

  messageConnection = '正在连接下位机';

  status = status;
}
</script>

<style lang="stylus">
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}
</style>
