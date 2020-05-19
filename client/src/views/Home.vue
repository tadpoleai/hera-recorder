<template lang="pug">
.home
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

  van-cell-group(title="数据")
    van-cell(
      v-if="status.remoteStatus.started"
      title="数据监视"
      is-link
      to="monitor")
    van-cell(
      v-else
      title="数据管理"
      is-link
      to="storage")

  van-cell-group(title="采集设置")
    van-cell(
      title="传感器配置"
      :is-link="!status.remoteStatus.started"
      :value="profileName"
      @click="clickProfile()"
    )
    van-cell(title="在线建图" center)
      van-switch(
        slot="right-icon"
        v-model="status.local.operatorInfo.slam"
        :disabled="status.remoteStatus.started"
        size="24")
    van-field(
      v-model="status.local.operatorInfo.operatorName"
      :disabled="status.remoteStatus.started"
      placeholder="请输入采集人"
      label="采集人"
      input-align="right")
    van-field(
      v-model="status.local.operatorInfo.place"
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
    if (checked) {
      if (!this.status.local.operatorInfo.operatorName) {
        return Toast.fail({
          message: '请输入采集人'
        });
      }

      if (!this.status.local.operatorInfo.place) {
        return Toast.fail({
          message: '请输入采集地点'
        });
      }

      this.isSwitchStartLoading = true;
      Api.start(this.status.local.operatorInfo).then(result => {
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
      this.isSwitchStartLoading = true;
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
