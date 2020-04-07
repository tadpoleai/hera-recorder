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
      van-cell(title="列表为空")
    //- For deviceList
    van-swipe-cell(v-for="(device, index) in status.local.profileToEdit.devices")
      van-cell(clickable @click="clickEditDevice(index)") {{device.type}}/{{device.name}}
      template(slot="right")
        van-button(square type="danger" text="删除" @click="clickDeleteDevice(index)")

  van-dialog(v-model="showDialog" :title="titleDialog"
    @confirm="clickConfirmDevice")
    van-cell-group(title="基本信息")
      template(v-if="isAddOrEditDevice")
        van-cell(title="类型" required center)
          van-picker(:columns="deviceTypeColumns" ref="deviceTypePicker"
            visible-item-count="2.5" swipe-duration="300"
            @change="changeDeviceType")
      template(v-else)
        van-cell(title="传感器类型" :value="deviceToEdit.type")

      van-field(v-model="deviceToEdit.name" required input-align="right"
        placeholder="请输入传感器名" label="传感器名")

      van-cell(title="转发数据")
        van-switch(v-model="deviceToEdit.forward" slot="right-icon" size="24")

      van-cell-group(title="参数")
        van-field(v-for="(param, index) in deviceToEdit.essentialParameters"
          v-model="param.value"
          required input-align="right"
          :label="param.type")
        van-field(v-for="(param, index) in deviceToEdit.optionalParameters"
          v-model="param.value"
          input-align="right"
          :label="param.type")

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast } from 'vant';

@Component({})
export default class ProfileEdit extends Vue {
  async clickNavBack() {
    if (this.status.local.profileAddOrEdit) {
      if (this.status.local.profileToEdit.name) {
        this.status.local.profiles.push(Object.assign(this.status.local.profileToEdit));
      }
    }
    const result = await Api.updateProfiles();
    if (result.error !== 0) {
      Toast.fail(Api.formatResult(result));
    } else {
      Toast.success('配置更新成功');
    }
    this.$router.back();
  }

  clickAddDevice() {
    this.isAddOrEditDevice = true;
    this.deviceTypeColumns = [''];
    console.log(this.status.remoteStatus.meta);
    this.status.remoteStatus.meta.deviceTypeMetas.forEach(typeMeta => this.deviceTypeColumns.push(typeMeta.name));
    console.log(this.deviceTypeColumns);
    this.showDialog = true;
    this.deviceToEdit = new Hera.Device({
      type: '',
      name: '',
      essentialParameters: [],
      optionalParameters: [],
      forward: false
    });
    this.$refs.deviceTypePicker.setColumnIndex(0);
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
    this.showDialog = true;
    this.deviceToEdit = this.status.local.profileToEdit.devices[index];
  }

  clickDeleteDevice(index: number) {
    this.status.local.profileToEdit.devices.splice(index, 1);
  }

  changeDeviceType(picker: any, value: string, index: number) {
    this.deviceToEdit.type = value;
    this.deviceToEdit.essentialParameters = [];
    this.deviceToEdit.optionalParameters = [];
    if (index > 0) {
      const meta = this.status.remoteStatus.meta.deviceTypeMetas[index - 1];
      meta.essentialParameterTypes.forEach(param =>
        this.deviceToEdit.essentialParameters.push(new Hera.Parameter({ type: param, value: '' }))
      );
      meta.optionalParameterTypes.forEach(param =>
        this.deviceToEdit.optionalParameters.push(new Hera.Parameter({ type: param, value: '' }))
      );
    }
  }

  showDialog = false;

  isAddOrEditDevice = false;

  status = status;

  deviceToEdit = new Hera.Device({
    type: '',
    name: '',
    essentialParameters: [],
    optionalParameters: [],
    forward: false
  });

  get titleDialog() {
    return this.isAddOrEditDevice ? '新增传感器' : '编辑传感器';
  }

  deviceTypeColumns: Array<string> = [''];
}
</script>
