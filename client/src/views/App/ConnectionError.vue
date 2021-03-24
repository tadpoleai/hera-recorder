<template lang="pug">
van-dialog(
  title="连接错误"
  :value="isConnectionErrored"
  :message="connectionErrorReason"
  theme="round-button"
  confirm-button-text="刷新"
  :beforeClose="refreshPage"
)

</template>

<script lang="ts">
import { Component, Vue, Watch } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';

const MainModule = namespace('Main');

@Component({})
export default class ConnectionError extends Vue {
  @MainModule.State isConnectionErrored;

  @MainModule.State connectionErrorReason;

  refreshPage() {
    console.log('A');
    window.location.reload(true);
  }
}
</script>

<style lang="stylus">
.checked-icon
  margin-left: 2px
</style>
