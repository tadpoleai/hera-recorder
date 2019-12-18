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
      <v-card-title>{{$t('Profile.title')}}</v-card-title>

      <v-card-text>
        {{$t('Profile.profiles')}}
        <v-tabs show-arrows>
          <v-tabs-slider></v-tabs-slider>
          <v-tab
            v-for="(profile, index) in daemonStatus.profiles"
            :key="'profile-'+index"
            :href="'#profile-' + index"
          >{{profile.name}}</v-tab>

          <v-btn
            v-show="daemonStatus.connected"
            :disabled="!editable"
            class="success white--text"
            @click="btnAddProfileHandler"
          >
            {{$t('Actions.addProfile')}}
            <v-icon right dark>mdi-plus-one</v-icon>
          </v-btn>

          <v-tab-item
            v-for="(profile, index) in daemonStatus.profiles"
            :key="'profile-'+index"
            :value="'profile-' + index"
          >
            <!-- Profile Name -->
            <v-card-text>{{$t('Profile.profileName')}}</v-card-text>
            <v-row class="ml-4 mr-4">
              <v-col cols="auto" class="pa-0 mr-auto">
                <v-text-field
                  :disabled="!editable"
                  class="ma-0 pa-0"
                  v-model="profile.name"
                  :rules="[rules.required, rules.normalName]"
                ></v-text-field>
              </v-col>
              <v-col cols="auto" class="ma-0 pa-0">
                <v-btn
                  :disabled="!editable"
                  class="error white--text"
                  :key="'delete-' + index"
                  @click="btnDeleteProfileHandler(index)"
                >
                  {{$t('Actions.deleteProfile')}}
                  <v-icon right dark>mdi-delete</v-icon>
                </v-btn>
              </v-col>
            </v-row>

            <!-- Device List -->
            <v-card-text>
              {{$t('Profile.deviceList')}}
              <device-list-edit :devices="profile.devices" :editable="editable" />
            </v-card-text>

            <v-divider />

            <!-- Storage Path -->
            <v-card-text>{{$t('Storage.path')}}</v-card-text>
            <v-card-text>
              <v-text-field
                v-if="!acquisitionRunning"
                v-model="storagePath"
                :rules="[rules.required, rules.onlyIdentifier]"
                :counter="maxStoragePathLength"
                :disabled="!editable"
                outlined
                :hint="$t('Storage.Path.hint')"
                :label="$t('Storage.Path.label')"
              ></v-text-field>
              <v-text-field
                v-else
                v-model="daemonStatus.status.folder"
                disabled
                :counter="maxStoragePathLength"
                outlined
                :hint="$t('Storage.Path.hint')"
                :label="$t('Storage.Path.label')"
              ></v-text-field>
            </v-card-text>

            <!-- Actions -->
            <v-card-actions>
              <v-btn
                :disabled="!editable"
                :loading="btnUpdateProfilePending"
                class="primary white--text"
                @click="btnUpdateProfileHandler"
              >
                {{$t('Actions.updateProfile')}}
                <v-icon right dark>mdi-upload</v-icon>
              </v-btn>
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
                {{$t('Actions.stop')}}
                <v-icon right dark>mdi-stop</v-icon>
              </v-btn>

              <v-btn
                v-if="btnStartShow"
                :loading="btnStartPending"
                class="primary white--text"
                @click="btnStartHandler(profile.devices)"
              >
                {{$t('Actions.start')}}
                <v-icon right dark>mdi-play</v-icon>
              </v-btn>
            </v-card-actions>
          </v-tab-item>
        </v-tabs>
      </v-card-text>
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
  getProfile,
  updateProfile,
  formatResult,
  toDeviceInfo,
  IParameter,
  IDevice,
  IDeviceInfo,
  ISimpleResult,
} from '@/core/daemonApi';
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

  updateStoragePath() {
    let date = new Date();
    date = new Date(date.getTime() - date.getTimezoneOffset() * 60000);
    this.storagePath = `${date
      .toISOString()
      .replace(/[\D]/g, '')
      .slice(0, 14)}_`;
  }

  mounted() {
    this.updateStoragePath();
  }

  maxStoragePathLength = '64';

  // Snackbar from result displaying
  snackbar = {
    show: false,
    text: '',
    color: 'success',
    multiLine: false,
    vertical: false,
  };

  get acquisitionRunning(): boolean {
    return this.daemonStatus.status.inited;
  }

  get editable(): boolean {
    return !this.btnStartPending && !this.daemonStatus.status.inited;
  }

  btnStartPending = false;

  btnStopPending = false;

  btnRecordPending = false;

  btnGetProfilePending = false;

  btnUpdateProfilePending = false;

  get btnStartShow(): boolean {
    return this.btnStartPending || !this.daemonStatus.status.inited;
  }

  get btnStopShow(): boolean {
    return this.btnStopPending || this.daemonStatus.status.inited;
  }

  get btnRecordShow(): boolean {
    return this.btnRecordPending || this.daemonStatus.status.inited;
  }

  get btnRecordText(): string {
    return this.daemonStatus.status.record ? 'Actions.pause' : 'Actions.record';
  }

  get btnRecordIcon(): string {
    return this.daemonStatus.status.record ? 'mdi-pause' : 'mdi-record';
  }

  btnStartHandler(devices: Array<IDevice>) {
    console.log('start');
    this.btnStartPending = true;
    start(devices, this.storagePath).then((result: ISimpleResult) => {
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
      this.updateStoragePath();
    });
  }

  btnRecordHandler() {
    console.log('record');
    this.btnRecordPending = true;
    record(!this.daemonStatus.status.record).then((result: ISimpleResult) => {
      this.resultHandler(result);
      this.btnRecordPending = false;
    });
  }

  btnAddProfileHandler() {
    daemonStatus.profiles.push({
      name: 'New Profile',
      devices: [],
    });
  }

  btnDeleteProfileHandler(index: number) {
    daemonStatus.profiles.splice(index, 1);
  }

  btnUpdateProfileHandler() {
    console.log('updateProfile');
    this.btnUpdateProfilePending = true;
    updateProfile().then((result: ISimpleResult) => {
      this.resultHandler(result);
      this.btnUpdateProfilePending = false;
    });
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
