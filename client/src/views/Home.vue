<template lang="pug">
.home
  AcquisitionControl

  van-cell(
    title="数据监视"
    :is-link="started"
    @click="$router.push('monitor')"
  )

  DiskUsage(
    :refresh="true"
  )

  AcquisitionSetting

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Getter, namespace } from 'vuex-class';

import AcquisitionControl from '@/views/Home/AcquisitionControl.vue';
import AcquisitionSetting from '@/views/Home/AcquisitionSetting.vue';
import DiskUsage from '@/components/DiskUsage.vue';
import JpegImage from '@/components/JpegImage.vue';

const AcquisitionControlModule = namespace('AcquisitionControl');

@Component({
  components: { AcquisitionControl, AcquisitionSetting, DiskUsage, JpegImage }
})
export default class Home extends Vue {
  @AcquisitionControlModule.Getter started;
}
</script>
