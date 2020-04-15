<template lang="pug">
.home
  van-nav-bar(title="HERA采集软件")

  van-cell-group(title="控制")
    van-cell(title="连接传感器" center)
      van-switch(
        slot="right-icon"
        :value="status.remoteStatus.started"
        @input="onClickSwitchStart"
        :loading="isSwitchStartLoading"
        size="24")
    van-cell(
      v-show="status.remoteStatus.started"
      title="录制数据" center)
      van-switch(
        slot="right-icon"
        :value="status.remoteStatus.recording"
        @input="onClickSwitchRecord"
        :loading="isSwitchRecordLoading"
        size="24")

  van-cell-group(
    v-if="status.remoteStatus.started"
    title="监视")
    van-cell(
      title="监视数据"
      is-link
      to="monitor")

  van-cell-group(title="设置")
    van-cell(
      title="配置"
      :is-link="!status.remoteStatus.started"
      :value="profileName"
      @click="clickProfile()"
    )
    van-cell(title="在线建图" center)
      van-switch(
        slot="right-icon"
        v-model="status.local.useSlam"
        :disabled="status.remoteStatus.started"
        size="24")
    van-field(
      v-model="status.local.executor"
      :disabled="status.remoteStatus.started"
      placeholder="请输入采集人"
      label="采集人"
      input-align="right")
    van-field(
      v-model="status.local.place"
      :disabled="status.remoteStatus.started"
      placeholder="请输入采集地点"
      label="采集地点"
      input-align="right")
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast } from 'vant';

@Component({})
export default class Home extends Vue {
  onClickSwitchStart(checked: boolean) {
    this.isSwitchStartLoading = true;
    if (checked) {
      Api.start(this.status.local.executor + '_' + this.status.local.place, this.status.local.useSlam).then(result => {
        if (result.error !== 0) {
          Toast.fail({
            message: Api.formatResult(result),
            duration: 0,
            closeOnClick: true
          });
        } else {
          Toast.success('操作成功');
        }
        this.isSwitchStartLoading = false;
      });
    } else {
      Api.stop().then(result => {
        this.isSwitchStartLoading = false;
      });
    }
  }

  onClickSwitchRecord(checked: boolean) {
    this.isSwitchRecordLoading = true;
    Api.record(checked).then(result => {
      if (result.error !== 0) {
        Toast.fail({
          message: Api.formatResult(result),
          duration: 0,
          closeOnClick: true
        });
      } else {
        Toast.success('操作成功');
      }
      this.isSwitchRecordLoading = false;
    });
  }

  isSwitchStartLoading = false;

  isSwitchRecordLoading = false;

  clickProfile() {
    if (!status.remoteStatus.started) {
      this.$router.push('profile');
    }
  }

  status = status;

  get profileName(): string {
    const profile = this.status.local.profiles[this.status.local.profileIndex];
    if (profile) {
      return profile.name;
    }
    return '';
  }
}
</script>
