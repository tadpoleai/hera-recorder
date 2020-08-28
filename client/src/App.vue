<template lang="pug">
div
  van-sticky
    van-nav-bar(
      :title="'HERA ' + $router.history.current.name"
      :left-arrow="$router.history.current.path != '/'"
      @click-left="onClickNavBack()"
      @click-right="onClickInfo()"
    )
      van-icon(
        slot="right"
        name="question-o"
        size="24px"
      )

    ControlStatusBar

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
import ControlStatusBar from '@/views/App/ControlStatusBar.vue';

const MetaModule = namespace('Meta');

@Component({
  components: { ConnectionError, ControlStatusBar }
})
export default class ProfileEdit extends Vue {
  // Actions
  @MetaModule.Action fetchMeta;

  created() {
    this.fetchMeta();
    // (this.$router as any).history.current!.path !== '/' && this.$router.replace({ path: '/' });
  }

  onClickNavBack() {
    if ((this.$router! as any).history!.current.path != '/') {
      this.$router.back();
    }
  }

  onClickInfo() {
    Dialog({
      title: 'HERA采集软件',
      message: '客户端版本\n' + GitVersion + '\n版权信息\nCopyright 2018 Wayz.ai. All Rights Reserved.',
      messageAlign: 'left',
      theme: 'round-button'
    });
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
