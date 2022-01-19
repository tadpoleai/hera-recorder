<template lang="pug">
.home
  AcquisitionControl

  van-cell(title='数据监视', :is-link='started', @click='onClickMonitorLink')

  AcquisitionSetting

  van-cell-group(title='数据管理')
    DiskUsage

    van-cell(@click='onClickStorageLink', is-link) 机内数据

  Preference
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Getter, namespace } from 'vuex-class';

import AcquisitionControl from '@/views/Home/AcquisitionControl.vue';
import AcquisitionSetting from '@/views/Home/AcquisitionSetting.vue';
import Preference from '@/views/Home/Preference.vue';
import DiskUsage from '@/components/DiskUsage.vue';

const AcquisitionControlModule = namespace('AcquisitionControl');

@Component({
  components: { AcquisitionControl, AcquisitionSetting, DiskUsage, Preference }
})
export default class Home extends Vue {
  @AcquisitionControlModule.Getter started;

  onClickMonitorLink() {
    if (this.started) {
      this.$router.push('monitor');
    }
  }

  onClickStorageLink() {
    this.$router.push('storage');
  }

  onClickPreferenceLink() {
    this.$router.push('preference');
  }

  activeNames = '1';
}
</script>
