<template>
  <div>
    <md-card id="control">
      <md-card-header>
        <div class="md-title">{{$t('Control')}}</div>
      </md-card-header>
      <md-card-content>
        <md-subheader>{{$t('Action')}}</md-subheader>
        <div class="md-layout md-gutter md-alignment-center-center">
          <div
            v-for="action in controlActions"
            :key="action.name"
            class="md-layout md-layout-item md-size-20 md-alignment-center-center"
          >
            <md-button class="md-raised control-button md-layout-item">
              <md-icon
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
          <div class="md-layout md-layout-item md-size-20 md-alignment-center-center">
            <md-button class="md-raised control-button md-layout-item">
              <md-icon class="md-size-2x" :md-src="require('@/assets/icons/folder.svg')"></md-icon>
              <span class="md-list-item-text">{{$t("Set Workspace")}}</span>
            </md-button>
          </div>
          <div class="md-layout md-layout-item md-size-60 md-alignment-center-center">
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
        TODO
      </md-card-content>
    </md-card>
  </div>
</template>

<script>
export default {
  name: 'acquisition',
  data() {
    return {
      controlActions: [
        { icon: 'on', name: 'Start' },
        { icon: 'stop', name: 'Stop' },
        { icon: 'play', name: 'Record' },
        { icon: 'pause', name: 'Pause' },
      ],
      folder: '2019_09_27',
    };
  },
};
</script>

<style lang="scss" scoped>
.control-button {
  height: 90px;
}
</style>
