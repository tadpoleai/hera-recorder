<template lang="pug">
div
  van-cell-group
    template(slot="title")
      div(style="display: flex; justify-content: space-between;")
        span 基本信息
        span
          van-icon.title-right-icon(
            name="sign"
            @click="onClickSaveProfile"
            size="24px"
            v-bind:class="{ 'enabled-icon': profileEdited }"
          )
    CheckedField(
      label="配置名"
      :value="profile.name"
      @input="setProfileName"
      regex=".+"
      placeholder="请输入配置名"
      input-align="right"
    )
    CheckedField(
      label="创建人"
      :value="profile.author"
      @input="setProfileAuthor"
      regex=".+"
      placeholder="请输入创建人"
      input-align="right"
    )

  van-popup(
    v-model="isShowSelectDeviceType"
    round
    position="bottom"
  )
    van-cell-group(title="选择添加的传感器类型")
      van-collapse(
        v-model="selectedDeviceCategory"
        accordion
      )
        van-collapse-item(
          v-for="category in categoriedDeviceTypes"
          :title="category.label"
          :name="category.value"
        )
          van-collapse(
            v-model="selectedDeviceType"
            accordion
          )
            van-collapse-item(
              v-for="device in category.children"
              :title="device.label"
              :name="device.value"
            )
              p(
                v-for="line in String(device.comment).split('\\n')"
              ) {{line}}
              van-button(
                type="primary" block
                @click="onSelectDeviceType(device.value)"
              ) 添加

  van-cell-group
    template(slot="title")
      div(style="display: flex; justify-content: space-between;")
        span 传感器列表
        span
          van-icon.title-middle-icon.enabled-icon(
            v-if="!isDeviceListEmpty"
            name="edit"
            @click="onClickListEdit"
            size="24px"
            v-bind:class="{ 'active-icon': isDeviceListEditMode }"
          )
          van-icon.title-right-icon.enabled-icon(
            name="add-o"
            @click="onClickAddDevice"
            size="24px"
          )

    template(v-if="isDeviceListEmpty")
      van-empty(description="传感器列表为空")
    template(v-else)
      template(v-if="isDeviceListEditMode")
        van-cell(
          v-for="(device, deviceIndex) in profile.devices"
          :value="device.type + '/' + device.name"
          center
        )
          template(
            slot="right-icon"
          )
            van-icon.title-right-icon(
              name="arrow-up"
              :color="deviceIndex != 0 ? '#1989fa' : '#FFF'"
              @click="moveDeviceIndex({deviceIndex, direction: -1})"
              size="24px"
            )
            van-icon.title-right-icon(
              name="arrow-down"
              :color="deviceIndex != profile.devices.length - 1 ? '#1989fa' : '#FFF'"
              @click="moveDeviceIndex({deviceIndex, direction: +1})"
              size="24px"
            )
            van-icon.title-right-icon(
              name="delete-o"
              color="red"
              @click="deleteDeviceByIndex(deviceIndex)"
              size="24px"
            )
      van-collapse(
        v-else
        v-model="activeDeviceIndex"
        accordion
      )
        van-collapse-item(
          v-for="(device, deviceIndex) in profile.devices"
          :title="device.type + '/' + device.name"
        )
          //- Item Panel
          van-cell-group(title="基本")
            van-cell(
              title="传感器类型"
              :value="device.type"
            )
            CheckedField(
              label="传感器名"
              :value="device.name"
              @input="setDeviceNameByIndex({deviceIndex, name: $event})"
              regex="^[a-zA-Z0-9]+$"
              placeholder="请输入传感器名(英数)"
              input-align="right"
            )
            van-cell(title="转发数据")
              van-switch(
                slot="right-icon" 
                :value="device.forward"
                @input="setDeviceForwardByIndex({deviceIndex, forward: $event})"
                size="24px"
              )
          van-cell-group(title="参数")
            Parameters(
              :deviceType="device.type"
              :values="device.parameters"
              @syncInput="setDeviceParameterByIndex({deviceIndex, parameterType: $event.type, parameterValue: $event.value})"
            )

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';
import { Dialog } from 'vant';

import CheckedField from '@/components/CheckedField.vue';
import Parameter from '@/components/Parameter.vue';
import Parameters from '@/components/Parameters.vue';

const ProfileEditModule = namespace('ProfileEdit');
const MetaModule = namespace('Meta');
const AcquisitionSettingModule = namespace('AcquisitionSetting');

@Component({
  components: { CheckedField, Parameter, Parameters }
})
export default class ProfileEdit extends Vue {
  @ProfileEditModule.State profile;
  @ProfileEditModule.State profileEdited;
  @MetaModule.State deviceRulesMap;

  // Getters
  @MetaModule.Getter deviceTypes;
  @MetaModule.Getter deviceParameterRuleMapMap;
  @MetaModule.Getter categoriedDeviceTypes;

  @AcquisitionSettingModule.Action finishEditingProfileAndSave;

  @ProfileEditModule.Action setProfileName;
  @ProfileEditModule.Action setProfileAuthor;

  @ProfileEditModule.Action addDeviceByType;
  @ProfileEditModule.Action deleteDeviceByIndex;
  @ProfileEditModule.Action moveDeviceIndex;

  @ProfileEditModule.Action setDeviceNameByIndex;
  @ProfileEditModule.Action setDeviceForwardByIndex;
  @ProfileEditModule.Action setDeviceParameterByIndex;

  @ProfileEditModule.Getter profileValid;

  async beforeDestroy() {
    if (this.profileEdited) {
      await this.finishEditingProfileAndSave();
    }
  }

  get deviceTypesVantActions() {
    return this.deviceTypes.map(type => {
      return {
        name: type
      };
    });
  }

  get isDeviceListEmpty() {
    return this.profile.devices.length === 0;
  }

  async onClickSaveProfile() {
    await this.finishEditingProfileAndSave();
    this.$router.back();
  }

  onClickListEdit() {
    this.isDeviceListEditMode = !this.isDeviceListEditMode;

    if (this.isDeviceListEditMode) {
      this.activeDeviceIndex = '';
    }
  }

  onClickAddDevice() {
    this.isShowSelectDeviceType = true;
  }

  onSelectDeviceType(deviceType: string) {
    this.isDeviceListEditMode = false;
    this.addDeviceByType(this.selectedDeviceType);
    this.selectedDeviceCategory = '';
    this.selectedDeviceType = '';
    this.isShowSelectDeviceType = false;
    this.activeDeviceIndex = this.profile.devices.length - 1;
  }

  isDeviceListEditMode = false;

  isShowSelectDeviceType = false;

  selectedDeviceCategory = '';

  selectedDeviceType = '';

  activeDeviceIndex: number | string = '';

  async onClickNavBack() {
    if (!this.profileValid.valid) {
      try {
        await Dialog.confirm({
          title: '提示',
          message: '配置有如下错误:\n' + this.profileValid.reason + '\n确定要后退吗?',
          theme: 'round-button'
        });

        return true;
      } catch {
        return false;
      }
    } else {
      return true;
    }
  }
}
</script>

<style lang="stylus">
.title-right-icon
  margin-left 4px
  margin-right 4px

.title-middle-icon
  margin-right 22px
  margin-left 2px

.enabled-icon
  color #1989fa !important

.active-icon
  color red !important
</style>
