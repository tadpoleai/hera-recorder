<template>
  <v-container fluid justify-center class="pa-0">
    <v-card class="mb-4 pa-2">
      <!-- Profile -->
      <v-card-title>{{$t('Acquisition.profile')}}</v-card-title>

      <!-- Device List -->
      <v-card-text>{{$t('Acquisition.Profile.deviceList')}}</v-card-text>
      <v-card-text>
        <device-list-edit :devices="devices" :deviceChangeDisabled="deviceChangeDisabled" />
      </v-card-text>
      <v-divider />

      <!-- Storage Path -->
      <v-card-text>{{$t('Acquisition.Profile.storagePath')}}</v-card-text>
      <v-card-text>
        <v-text-field
          v-model="storagePath"
          :rules="[rules.required, rules.onlyIdentifier]"
          :counter="maxStoragePathLength"
          outlined
          :hint="$t('Acquisition.Profile.StoragePath.hint')"
          :label="$t('Acquisition.Profile.StoragePath.label')"
        ></v-text-field>
      </v-card-text>

      <!-- Actions -->
      <v-card-actions>
        <v-spacer />
        <v-btn
          :loading="btnStopPending"
          v-if="btnStopShow"
          class="primary white--text"
          @click="btnStopHandler"
        >
          {{$t('Acquisition.Profile.Actions.stop')}}
          <v-icon right dark>mdi-stop</v-icon>
        </v-btn>

        <v-btn
          :loading="btnStartPending"
          v-if="btnStartShow"
          class="primary white--text"
          @click="btnStartHandler"
        >
          {{$t('Acquisition.Profile.Actions.start')}}
          <v-icon right dark>mdi-play</v-icon>
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import inputRules from '@/core/inputRules';
import {
  daemonStatus,
  start,
  stop,
  IParameter,
  IDevice,
  ISimpleResult,
} from '@/core/tronApi';
import deviceDefine from '@/core/deviceDefine';
import DeviceListEdit from '@/views/Acquisition/DeviceListEdit.vue';

@Component({
  components: {
    DeviceListEdit,
  },
})
export default class Acquisition extends Vue {
  storagePath: string = '2019';

  maxStoragePathLength = '64';

  rules = inputRules;

  daemonStatus = daemonStatus;

  deviceDefine = deviceDefine;

  devices: Array<IDevice> = [
    {
      type: 'dummy',
      name: 'dummy0',
      parameters: [
        { name: 'dummyRate', value: '1' },
        { name: 'dummyValue', value: '1' },
      ],
    },
    {
      type: 'lidar',
      name: 'velodyne0',
      parameters: [
        { name: 'ipAddress', value: '10.0.0.1' },
        { name: 'port', value: '8080' },
      ],
    },
  ];

  get deviceChangeDisabled() {
    return this.btnStartPending || !this.daemonStatus.status.can_start;
  }

  btnStartPending = false;

  btnStopPending = false;

  get btnStartShow() {
    return this.btnStartPending || this.daemonStatus.status.can_start;
  }

  get btnStopShow() {
    return this.btnStopPending || this.daemonStatus.status.can_stop;
  }

  btnStartHandler() {
    console.log('start');
    this.btnStartPending = true;
    start(this.devices, this.storagePath).then((result: ISimpleResult) => {
      console.log(result);
      this.btnStartPending = false;
    });
  }

  btnStopHandler() {
    console.log('stop');
    this.btnStopPending = true;
    stop().then((result: ISimpleResult) => {
      console.log(result);
      this.btnStopPending = false;
    });
  }
}
</script>
