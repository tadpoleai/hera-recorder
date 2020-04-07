<template lang="pug">
div
  router-view(v-if="status.remoteConnected")
  template(v-else)
    van-nav-bar(title="正在连接下位机")
    van-cell
      van-skeleton(title row="8")
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { status } from '@/api';

@Component({})
export default class ProfileEdit extends Vue {
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
