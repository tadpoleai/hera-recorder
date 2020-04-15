<template lang="pug">
div
  van-nav-bar(title="编辑配置" left-arrow @click-left="clickNavBack()")

  van-cell-group(title="基本信息")
    van-field(v-model="status.local.profileToEdit.author"
      placeholder="请输入创建人" label="创建人")
    van-field(v-model="status.local.profileToEdit.name"
    placeholder="请输入配置名" label="配置名" required)

  van-cell-group
    template(slot="title")
      div(style="display: flex; justify-content: space-between;")
        span 传感器列表
        van-icon(type="info" name="more-o" size="large" @click="clickAddDevice()")
    template(v-if="status.local.profileToEdit.devices.length === 0")
      van-empty(description="传感器列表为空")
    //- For deviceList
    van-swipe-cell(v-for="(device, index) in status.local.profileToEdit.devices")
      van-cell(
        clickable
        center
        @click="clickEditDevice(index)"
        :value="device.type + '/' + device.name")
        template(slot="icon")
          van-icon(v-if="checkDevice(device)" name="passed" color="green" size="large")
          van-icon(v-else name="warning-o" color="red" size="large")
      template(slot="right")
        van-button(square type="danger" text="删除" @click="clickDeleteDevice(index)")

  van-action-sheet(
    v-model="showSelectDeviceType"
    :actions="status.remoteStatus.meta.deviceTypeMetas"
    description="传感器类型"
    cancel-text="取消"
    @select="clickSelectDeviceType")

  van-dialog(v-model="showEditDevice" :title="titleEditDevice"
    @confirm="clickConfirmDevice")
    van-cell-group(title="基本信息")
      van-cell(title="传感器类型" :value="deviceToEdit.type")

      van-field(v-model="deviceToEdit.name" required input-align="right"
        placeholder="请输入传感器名" label="传感器名")

      van-cell(title="转发数据")
        van-switch(
          slot="right-icon" 
          v-model="deviceToEdit.forward"
          size="24")

      van-cell-group(title="参数")
        van-field(v-for="(param, index) in deviceToEdit.essentialParameters"
          v-model="param.value"
          :key="'e' + index"
          required input-align="right"
          :label="param.type")
        van-field(v-for="(param, index) in deviceToEdit.optionalParameters"
          v-model="param.value"
          :key="'o' + index"
          input-align="right"
          :label="param.type")

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast } from 'vant';

@Component({})
export default class ProfileEdit extends Vue {
  mounted() {
    this.deviceTypeMetas = this.status.remoteStatus.meta.deviceTypeMetas.slice();
  }

  async clickNavBack() {
    if (this.status.local.profileAddOrEdit) {
      if (this.status.local.profileToEdit.name) {
        this.status.local.profiles.push(Object.assign(this.status.local.profileToEdit));
      }
    }
    const result = await Api.updateProfiles();
    if (result.error !== 0) {
      Toast.fail({
        message: Api.formatResult(result),
        duration: 0,
        closeOnClick: true
      });
    } else {
      Toast.success('配置更新成功');
    }
    this.$router.back();
  }

  clickSelectDeviceType(meta: Hera.DeviceTypeMeta) {
    this.isAddOrEditDevice = true;
    this.showEditDevice = true;
    this.deviceToEdit = new Hera.Device({
      type: meta.name,
      name: '',
      essentialParameters: [],
      optionalParameters: [],
      forward: true
    });
    meta.essentialParameterTypes.forEach(param =>
      this.deviceToEdit.essentialParameters.push(new Hera.Parameter({ type: param, value: '' }))
    );
    meta.optionalParameterTypes.forEach(param =>
      this.deviceToEdit.optionalParameters.push(new Hera.Parameter({ type: param, value: '' }))
    );
    this.showSelectDeviceType = false;
  }

  clickAddDevice() {
    this.showSelectDeviceType = true;
  }

  clickConfirmDevice() {
    if (this.isAddOrEditDevice) {
      if (this.deviceToEdit.type != '') {
        this.status.local.profileToEdit.devices.push(Object.assign(this.deviceToEdit));
      }
    }
  }

  clickEditDevice(index: number) {
    this.isAddOrEditDevice = false;
    this.showEditDevice = true;
    this.deviceToEdit = this.status.local.profileToEdit.devices[index];
  }

  clickDeleteDevice(index: number) {
    this.status.local.profileToEdit.devices.splice(index, 1);
  }

  checkDevice(device: Hera.Device) {
    if (!device.name) {
      return false;
    }

    let ret = true;
    device.essentialParameters.forEach(param => {
      if (!param.value) {
        ret = false;
      }
    });
    return ret;
  }

  status = status;

  showSelectDeviceType = false;

  showEditDevice = false;

  isAddOrEditDevice = false;

  deviceTypeMetas = new Array<Hera.DeviceTypeMeta>();

  deviceToEdit = new Hera.Device({
    type: '',
    name: '',
    essentialParameters: [],
    optionalParameters: [],
    forward: false
  });

  get titleEditDevice() {
    return this.isAddOrEditDevice ? '新增传感器' : '编辑传感器';
  }

  deviceTypeColumns: Array<string> = [''];
}
</script>
