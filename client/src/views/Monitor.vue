<template lang="pug">
.monitor
  van-nav-bar(title="HERA监视数据")
  van-cell-group(title="")
    van-cell(title="连接传感器" center)
      van-switch(
        :value="status.remoteStatus.started"
        @input="onClickSwitchStart"
        :loading="isSwitchStartLoading"
        size="24")
    van-cell(
      v-show="status.remoteStatus.started"
      title="录制数据" center)
      van-switch(
        :value="status.remoteStatus.recording"
        @input="onClickSwitchRecord"
        :loading="isSwitchRecordLoading"
        size="24")
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast } from 'vant';

@Component({})
export default class Home extends Vue {
  status = status;
}
</script>
