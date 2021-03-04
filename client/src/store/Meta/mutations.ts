import { Vue } from 'vue-property-decorator';
import { MutationTree } from 'vuex';
import { DeviceDescription, MetaState } from './types';

export const mutations: MutationTree<MetaState> = {
  addDeviceDescription(state, payload: { name: string; description: DeviceDescription }) {
    Vue.set(state.deviceDescriptionMap, payload.name, payload.description);
  },

  setDaemonVersion(state, value: string) {
    state.daemonVersion = value;
  }
};
