<template>
  <v-container fluid justify-center class="pa-0">
    <device-list-monitor :devices="devices" />
  </v-container>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import {
  daemonStatus,
  formatResult,
  toDeviceInfo,
  IParameter,
  IDevice,
  IDeviceInfo,
  ISimpleResult,
} from '@/core/daemonApi';
import deviceDefine from '@/core/deviceDefine';
import DeviceListMonitor from '@/views/Monitor/DeviceListMonitor.vue';

@Component({
  components: {
    DeviceListMonitor,
  },
})
export default class Monitor extends Vue {
  // Definition from other
  daemonStatus = daemonStatus;

  deviceDefine = deviceDefine;

  get devices() {
    return toDeviceInfo(this.daemonStatus.status.devices);
  }

  get daemonRunning(): boolean {
    return this.daemonStatus.status.inited;
  }
}
</script>
