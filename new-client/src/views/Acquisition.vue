<template>
  <v-container fluid justify-center class="pa-0">
    <v-snackbar
      v-model="snackbar.show"
      top
      right
      :color="snackbar.color"
      :multi-line="snackbar.multiLine"
      :vertical="snackbar.vertical"
      :timeout="6000"
    >
      {{snackbar.text}}
      <v-btn text @click="snackbar.show = false">{{$t('Button.ok')}}</v-btn>
    </v-snackbar>

    <v-card class="mb-4 pa-2">
      <!-- Profile -->
      <v-card-title>{{$t('Acquisition.profile')}}</v-card-title>

      <!-- Device List -->
      <v-card-text>
        {{$t('Acquisition.Profile.deviceList')}}
        <device-list-edit :devices="devicesToShow" :editable="editable" />
      </v-card-text>
      <v-divider />

      <!-- Storage Path -->
      <v-card-text>{{$t('Acquisition.Profile.storagePath')}}</v-card-text>
      <v-card-text>
        <v-text-field
          v-if="!daemonRunning"
          v-model="storagePath"
          :rules="[rules.required, rules.onlyIdentifier]"
          :counter="maxStoragePathLength"
          :disabled="!editable"
          outlined
          :hint="$t('Acquisition.Profile.StoragePath.hint')"
          :label="$t('Acquisition.Profile.StoragePath.label')"
        ></v-text-field>
        <v-text-field
          v-else
          v-model="daemonStatus.status.storage_folder"
          disabled
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
          v-if="btnRecordShow"
          :loading="btnRecordPending"
          class="primary white--text"
          @click="btnRecordHandler"
        >
          {{$t(btnRecordText)}}
          <v-icon right dark>{{btnRecordIcon}}</v-icon>
        </v-btn>

        <v-btn
          v-if="btnStopShow"
          :loading="btnStopPending"
          class="primary white--text"
          @click="btnStopHandler"
        >
          {{$t('Acquisition.Profile.Actions.stop')}}
          <v-icon right dark>mdi-stop</v-icon>
        </v-btn>

        <v-btn
          v-if="btnStartShow"
          :loading="btnStartPending"
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
  record,
  formatResult,
  toDeviceInfo,
  IParameter,
  IDevice,
  IDeviceInfo,
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
  // Definition from other
  rules = inputRules;

  daemonStatus = daemonStatus;

  deviceDefine = deviceDefine;

  // Storage path
  storagePath = '';

  mounted() {
    let date = new Date();
    date = new Date(date.getTime() - date.getTimezoneOffset() * 60000);
    this.storagePath = `${date
      .toISOString()
      .replace(/[\D]/g, '')
      .slice(0, 14)}_`;
  }

  maxStoragePathLength = '64';

  // Devices
  devices: Array<IDevice> = [
    {
      type: 'Dummy',
      name: 'dummy0',
      parameters: [
        { type: 'DummyRate', value: '1' },
        { type: 'DummyValue', value: '1' },
      ],
    },
  ];

  // Snackbar from result displaying
  snackbar = {
    show: false,
    text: '',
    color: 'success',
    multiLine: false,
    vertical: false,
  };

  get daemonRunning(): boolean {
    return this.daemonStatus.status.can_stop;
  }

  get devicesToShow(): Array<IDevice> | Array<IDeviceInfo> {
    console.log(this.daemonStatus.status.devices);
    return this.daemonRunning
      ? toDeviceInfo(this.daemonStatus.status.devices)
      : this.devices;
  }

  get editable(): boolean {
    return !this.btnStartPending && this.daemonStatus.status.can_start;
  }

  btnStartPending = false;

  btnStopPending = false;

  btnRecordPending = false;

  get btnStartShow(): boolean {
    return this.btnStartPending || this.daemonStatus.status.can_start;
  }

  get btnStopShow(): boolean {
    return this.btnStopPending || this.daemonStatus.status.can_stop;
  }

  get btnRecordShow(): boolean {
    return (
      this.btnRecordPending
      || this.daemonStatus.status.can_record
      || this.daemonStatus.status.can_pause
    );
  }

  get btnRecordText(): string {
    return this.daemonStatus.status.is_record
      ? 'Acquisition.Profile.Actions.pause'
      : 'Acquisition.Profile.Actions.record';
  }

  get btnRecordIcon(): string {
    return this.daemonStatus.status.is_record ? 'mdi-pause' : 'mdi-record';
  }

  btnStartHandler() {
    console.log('start');
    this.btnStartPending = true;
    start(this.devices, this.storagePath).then((result: ISimpleResult) => {
      this.resultHandler(result);
      this.btnStartPending = false;
    });
  }

  btnStopHandler() {
    console.log('stop');
    this.btnStopPending = true;
    stop().then((result: ISimpleResult) => {
      this.resultHandler(result);
      this.btnStopPending = false;
    });
  }

  btnRecordHandler() {
    console.log('record');
    this.btnRecordPending = true;
    record(!this.daemonStatus.status.is_record).then(
      (result: ISimpleResult) => {
        this.resultHandler(result);
        this.btnRecordPending = false;
      },
    );
  }

  resultHandler(result: ISimpleResult) {
    console.log(result);
    this.snackbar.show = true;
    if (result.error !== 0) {
      this.snackbar.text = formatResult(result);
      this.snackbar.color = 'error';
      this.snackbar.multiLine = true;
      this.snackbar.vertical = true;
    } else {
      this.snackbar.text = String(this.$t('Result.success'));
      this.snackbar.color = 'success';
      this.snackbar.multiLine = false;
      this.snackbar.vertical = false;
    }
  }
}
</script>
