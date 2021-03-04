<template lang="pug">
van-dialog(
  title="连接错误"
  :value="isConnectionErrored"
  :message="connectionErrorReason"
  theme="round-button"
  confirm-button-text="刷新"
  :beforeClose="refreshPage"
  message-align="center"
)
  van-empty(image="network")
    span(slot="description") {{ connectionErrorReason }}
    div
      van-cell(value="请确认设备地址正确，或尝试重启设备")
      van-field(
        label="地址(URL)"
        :value="hostUrl"
        @input="setHostUrl"
      )
      van-field(
        label="端口(PORT)"
        :value="hostPort"
        type="digit"
        @input="setHostPort"
        )

</template>

<script lang="ts">
import { Component, Vue, Watch } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';

const MainModule = namespace('Main');

@Component({})
export default class ConnectionError extends Vue {
  @MainModule.Action refreshAll;

  @MainModule.State isConnectionErrored;
  @MainModule.State connectionErrorReason;

  @MainModule.State hostUrl;
  @MainModule.State hostPort;

  @MainModule.Mutation clearConnectionError;
  @MainModule.Mutation setHostUrl;
  @MainModule.Mutation setHostPort;

  refreshPage(action, done) {
    this.clearConnectionError();
    done();

    this.refreshAll();
    (this.$router as any).history.current!.path !== '/' && this.$router.replace({ path: '/' });
  }
}
</script>

<style lang="stylus">
.checked-icon
  margin-left: 2px
</style>
