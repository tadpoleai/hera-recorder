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
      <v-card class="pa-1">
        <v-card-text>
          <v-container class="ma-0 pa-0">
            <v-row no-gutters>
              <!-- Device Index -->
              <v-col cols="1">{{indexDevice + 1}}</v-col>
              <!-- Device Type -->
              <v-col cols="5" class="pa-1">{{device.type}}</v-col>
              <!-- Device Name -->
              <v-col cols="6" class="pa-1">{{device.name}}</v-col>
            </v-row>
          </v-container>

          <!-- Device Parameters -->
          {{$t('Devices.parameters')}}
          <v-container class="ma-0 pa-0">
            <v-row
              v-for="(param, indexParam) in device.parameters"
              :key="indexDevice+'-'+indexParam"
              no-gutters
            >
              <v-col cols="6" class="pa-1">{{param.type}}</v-col>
              <v-col cols="6" class="pa-1">{{param.value}}</v-col>
            </v-row>
          </v-container>
        </v-card-text>
      </v-card>
    </v-col>
  </v-row>
</template>

<script lang="ts">
import { Component, Vue, Prop } from 'vue-property-decorator';
import { IParameter, IDevice, IDeviceInfo } from '@/core/tronApi';
import deviceDefine from '@/core/deviceDefine';

@Component
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
    } if (this.$vuetify.breakpoint.smAndDown) {
      return 2;
    } if (this.$vuetify.breakpoint.mdAndDown) {
      return 3;
    }
    return 4;
  }
}
</script>
