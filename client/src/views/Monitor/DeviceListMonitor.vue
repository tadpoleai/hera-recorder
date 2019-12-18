<template>
  <v-row>
    <v-col
      v-for="(device, indexDevice) in devices"
      :key="indexDevice"
      cols="12"
      sm="6"
      md="4"
      lg="3"
    >
      <!-- For Device List -->
      <v-card :color="getCardColor(device)" class="pa-2">
        <v-card-title>
          <v-icon large left>{{deviceDefine.deviceIconTypes[device.type]}}</v-icon>
          {{device.type}}
        </v-card-title>

        <v-card-text>
          <v-row>
            <v-col cols="6" class="pa-1">{{$t('Acquisition.deviceName')}}</v-col>
            <v-col cols="6" class="pa-1">{{device.name}}</v-col>
          </v-row>
          
          <v-row>
            <v-col cols="6" class="pa-1">{{$t('Acquisition.volume')}}</v-col>
            <v-col cols="6" class="pa-1">{{device.volume}}</v-col>
          </v-row>

          <data-display :displayData="device.data" />
        </v-card-text>
      </v-card>
    </v-col>
  </v-row>
</template>

<script lang="ts">
import { Component, Vue, Prop } from "vue-property-decorator";
import { IParameter, IDevice, IDeviceInfo } from "@/core/daemonApi";
import deviceDefine from "@/core/deviceDefine";
import DataDisplay from "@/views/Monitor/DataDisplay.vue";

@Component({
  components: {
    DataDisplay
  }
})
export default class DeviceListEdit extends Vue {
  @Prop()
  devices!: Array<IDeviceInfo>;

  deviceDefine = deviceDefine;

  getIndex(indexInPage: number, props: any): number {
    return indexInPage + props.pagination.pageStart;
  }

  get devicesPerPage(): number {
    if (this.$vuetify.breakpoint.xs) {
      return 1;
    }
    if (this.$vuetify.breakpoint.smAndDown) {
      return 2;
    }
    if (this.$vuetify.breakpoint.mdAndDown) {
      return 3;
    }
    return 4;
  }

  getCardColor(device: IDeviceInfo): string {
    if (device.error === 0) {
      return "";
    }
    if (device.error === 700) {
      return "warn";
    }
    return "error";
  }
}
</script>
