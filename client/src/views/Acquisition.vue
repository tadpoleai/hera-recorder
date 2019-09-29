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
              class="md-raised control-button md-layout-item md-accent"
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
          <div class="md-layout md-layout-item md-alignment-center-center md-size-25">
            <md-icon class="md-layout-item md-size-2x" :md-src="require('@/assets/icons/folder.svg')"></md-icon>
          </div>
          <div class="md-layout md-layout-item md-alignment-center-center md-size-75">
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
              @click=""
              ><md-icon class="md-size-2x" :md-src="require('@/assets/icons/folder.svg')"></md-icon>
              <span class="md-list-item-text">{{$t("Apply Profile")}}</span>
            </md-button>
          </div>
        </div>
      </md-card-content>
    </md-card>

    <md-snackbar
      md-position="center"
      :md-duration="30000"
      :md-active.sync="pending.showSnacker"
      md-persistent
    >
      <span>{{pending.message}}</span>
      <md-button class="md-primary" @click="closeSnackbar()">CLOSE</md-button>
    </md-snackbar>
  </div>
</template>

<script>
import TronApi from '@/functions/tron_api';

export default {
  name: 'acquisition',
  computed: {
    showSnacker() {
      return true;
    },
    progressMode() {
      return this.pending.pending ? 'indeterminate' : 'determinate';
    },
    canNotStart() {
      return this.pending.pending || !this.serverInfo.connected || !this.serverInfo.status.can_start;
    },
    canNotStop() {
      return this.pending.pending || !this.serverInfo.connected || !this.serverInfo.status.can_stop;
    },
    canNotRecord() {
      return this.pending.pending || !this.serverInfo.connected || !this.serverInfo.status.can_record;
    },
    canNotPause() {
      return this.pending.pending || !this.serverInfo.connected || !this.serverInfo.status.can_pause;
    },
  },
  methods: {
    closeSnackbar() {
      this.pending.showSnacker = false;
    },
    setPending(onOff) {
      this.pending.pending = onOff;
    },
    returnHandler(ret) {
      this.pending.message = TronApi.formatError(ret);
      this.pending.showSnacker = true;
    },
    async clickStart() {
      this.setPending(true);
      const ret = await TronApi.start(
        [
          { type: 'dummy', name: 'dummy0', parameters: { dummyValue: '1', dummyRate: '1' } },
        ],
        this.folder,
      );
      this.setPending(false);
      this.returnHandler(ret);
    },
    async clickStop() {
      this.setPending(true);
      const ret = await TronApi.stop();
      this.setPending(false);
      this.returnHandler(ret);
    },
    async clickRecord() {
      this.setPending(true);
      const ret = await TronApi.recordOrPause(true);
      this.setPending(false);
      this.returnHandler(ret);
    },
    async clickPause() {
      this.setPending(true);
      const ret = await TronApi.recordOrPause(false);
      this.setPending(false);
      this.returnHandler(ret);
    },
  },
  data() {
    return {
      pending: {
        message: 'OK',
        showSnacker: true,
        pending: false,
      },
      serverInfo: TronApi.serverInfo,
      folder: new Date().toString(),
      controlActions: [
        {
          icon: 'on',
          name: 'Start',
          func: this.clickStart,
          disabled: () => this.canNotStart,
        },
        {
          icon: 'stop',
          name: 'Stop',
          func: this.clickStop,
          disabled: () => this.canNotStop,
        },
        {
          icon: 'play',
          name: 'Record',
          func: this.clickRecord,
          disabled: () => this.canNotRecord,
        },
        {
          icon: 'pause',
          name: 'Pause',
          func: this.clickPause,
          disabled: () => this.canNotPause,
        },
      ],
    };
  },
};
</script>

<style lang="scss" scoped>
.control-button {
  height: 90px;
}
</style>
