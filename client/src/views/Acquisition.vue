<template>
  <div>
    <md-card id="control">
      <md-progress-bar class="md-accent" :md-mode="progressMode" />
      <md-card-header>
        <div class="md-title">{{$t('Control')}}</div>
      </md-card-header>
      <md-card-content>
        <md-subheader>{{$t('Action')}}</md-subheader>
        <div class="md-layout md-gutter md-alignment-center-center">
          <div
            v-for="action in controlActions"
            :key="action.name"
            class="md-layout md-layout-item md-alignment-center-center"
          >
            <md-button
              class="md-raised control-button md-layout-item"
              @click="action.func()"
              :disabled="action.disabled()"
              ><md-icon
                class="md-size-2x"
                :md-src="require('@/assets/icons/' + action.icon + '.svg')"
              ></md-icon>
              <span class="md-list-item-text">{{$t(action.name)}}</span>
            </md-button>
          </div>
        </div>

        <md-divider />
        <md-subheader>{{$t('Workspace')}}</md-subheader>
        <div class="md-layout md-gutter md-alignment-center-center">
          <div class="md-layout md-layout-item md-alignment-center-center">
            <md-button
              class="md-raised control-button md-layout-item"
              @click="setStorage(folder)"
              :disabled="canNotSetFolder"
              ><md-icon class="md-size-2x" :md-src="require('@/assets/icons/folder.svg')"></md-icon>
              <span class="md-list-item-text">{{$t("Set Workspace")}}</span>
            </md-button>
          </div>
          <div class="md-layout md-layout-item md-alignment-center-center">
            <md-field class="md-layout-item md-size-90">
              <label>{{$t("Workspace")}}</label>
              <md-input v-model="folder" type="string"></md-input>
              <span class="md-helper-text">{{$t("Folder to save recorded data.")}}</span>
            </md-field>
          </div>
        </div>
      </md-card-content>
    </md-card>

    <md-card id="profile">
      <md-card-header>
        <div class="md-title">{{$t('Profile')}}</div>
      </md-card-header>
      <md-card-content>
        <md-subheader>{{$t('Profile')}}</md-subheader>
        <div class="md-layout md-gutter md-alignment-center-center">
          <div class="md-layout md-layout-item md-alignment-center-center">
            <md-button
              class="md-raised control-button md-layout-item"
              @click="createDevices()"
              ><md-icon class="md-size-2x" :md-src="require('@/assets/icons/folder.svg')"></md-icon>
              <span class="md-list-item-text">{{$t("Apply Profile")}}</span>
            </md-button>
          </div>
        </div>
      </md-card-content>
    </md-card>

    <md-snackbar
      md-position="center"
      :md-duration="progress.snackDuration"
      :md-active.sync="progress.snack"
      md-persistent
    >
      <span>{{progress.message}}</span>
      <md-button class="md-primary" @click="progress.snack = false">CLOSE</md-button>
    </md-snackbar>
  </div>
</template>

<script>
import TronApi from '@/functions/tron_api';
import ServerInfo from '@/functions/server_info';

const progress = {
  show: true,
  snack: false,
  snackDuration: 15000,
  message: 'OK',
};

const clickFunc = async (command) => {
  progress.show = true;
  const ret = await TronApi.control(command);
  progress.show = false;
  progress.message = TronApi.formatError(ret);
  progress.snack = true;
};

const setStorage = async (folder) => {
  progress.show = true;
  console.log(folder);
  const ret = await TronApi.setStorage(folder);
  progress.show = false;
  progress.message = TronApi.formatError(ret);
  progress.snack = true;
};

export default {
  name: 'acquisition',
  computed: {
    progressMode() {
      return this.progress.show ? 'indeterminate' : 'determinate';
    },
    canNotSetFolder() {
      !this.ServerInfo.information.can_set_storage || this.ServerInfo.isConnectionLost;
    },
  },
  methods: {
    createDevices: async (devices) => {
      progress.show = true;
      devices = [
        { type: 'dummy', name: 'dum0', parameters: { dummyRate: '1', dummyValue: '0xCC' } },
      ];
      const ret = await TronApi.createDevices(devices);
      progress.show = false;
      progress.message = TronApi.formatError(ret);
      progress.snack = true;
    },
  },
  data() {
    return {
      ServerInfo,
      progress,
      setStorage,
      controlActions: [
        {
          icon: 'on',
          name: 'Start',
          func: () => clickFunc(TronApi.ControlCommand.Start),
          disabled: () => !this.ServerInfo.information.can_start || this.ServerInfo.isConnectionLost,
        },
        {
          icon: 'stop',
          name: 'Stop',
          func: () => clickFunc(TronApi.ControlCommand.Stop),
          disabled: () => !this.ServerInfo.information.can_stop || this.ServerInfo.isConnectionLost,
        },
        {
          icon: 'play',
          name: 'Record',
          func: () => clickFunc(TronApi.ControlCommand.StartRecord),
          disabled: () => !this.ServerInfo.information.can_record || this.ServerInfo.isConnectionLost,
        },
        {
          icon: 'pause',
          name: 'Pause',
          func: () => clickFunc(TronApi.ControlCommand.PauseRecord),
          disabled: () => !this.ServerInfo.information.can_pause || this.ServerInfo.isConnectionLost,
        },
        {
          icon: 'stop',
          name: 'Reset',
          func: () => clickFunc(TronApi.ControlCommand.Reset),
          disabled: () => this.ServerInfo.isConnectionLost,
        },
      ],
      folder: new Date().toString(),
    };
  },
};
</script>

<style lang="scss" scoped>
.control-button {
  height: 90px;
}
</style>
