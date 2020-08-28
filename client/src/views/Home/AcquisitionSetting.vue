<template lang="pug">
van-cell-group(
  title="采集设置"
)
  van-cell(
    title="传感器配置"
    :value="currentProfileName"
    is-link
    @click="onClickProfileName()"
  )
  van-cell(
    title="在线建图"
    center
  )
    van-switch(
      slot="right-icon"
      :value="fetchedData.operatorInfo.slam"
      @input="setOperatorInfoSlam"
      size="24px"
    )
  CheckedField(
    placeholder="请输入采集操作员(英文)"
    :value="fetchedData.operatorInfo.operatorName"
    @input="setLocalOperatorInfoOperatorName"
    @change="updateOperatorInfoOperatorName"
    regex="^[a-zA-Z0-9]+$"
    label="采集操作员"
    input-align="right"
  )
  CheckedField(
    label="采集地点"
    :value="fetchedData.operatorInfo.place"
    @input="setLocalOperatorInfoPlace"
    @change="updateOperatorInfoPlace"
    regex="^[a-zA-Z0-9]+$"
    placeholder="请输入采集地点(英文)"
    input-align="right"
  )

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';

import CheckedField from '@/components/CheckedField.vue';

const AcquisitionSettingModule = namespace('AcquisitionSetting');

@Component({
  components: { CheckedField }
})
export default class Home extends Vue {
  @AcquisitionSettingModule.State fetchedData;

  // Getters
  @AcquisitionSettingModule.Getter currentProfileName;

  // Actions
  @AcquisitionSettingModule.Action fetch;

  // OperatorInfo
  @AcquisitionSettingModule.Action setOperatorInfoSlam;
  @AcquisitionSettingModule.Action setLocalOperatorInfoOperatorName;
  @AcquisitionSettingModule.Action updateOperatorInfoOperatorName;
  @AcquisitionSettingModule.Action setLocalOperatorInfoPlace;
  @AcquisitionSettingModule.Action updateOperatorInfoPlace;

  async created() {
    await this.fetch();
  }

  onClickProfileName() {
    this.$router.push('profiles');
  }
}
</script>
