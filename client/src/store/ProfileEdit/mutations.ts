import { Vue } from 'vue-property-decorator';
import { MutationTree } from 'vuex';
import { ProfileEditState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<ProfileEditState> = {
  setProfile(state, profile: Hera.Profile) {
    state.profile = profile;
    state.profileEdited = false;
  },

  setProfileName(state, name: string) {
    state.profile.name = name;
    state.profileEdited = true;
  },

  setProfileAuthor(state, author: string) {
    state.profile.author = author;
    state.profileEdited = true;
  },

  deleteDeviceByIndex(state, index: number) {
    state.profile.devices.splice(index, 1);
    state.profileEdited = true;
  },

  moveDeviceIndex(state, payload: { deviceIndex: number; direction: number }) {
    const src = payload.deviceIndex;
    const dst = src + payload.direction;
    const devices = state.profile.devices;
    if (src >= 0 && dst >= 0 && src < devices.length && dst < devices.length) {
      const deviceSrc = devices[src];
      const deviceDst = devices[dst];
      Vue.set(state.profile.devices, src, deviceDst);
      Vue.set(state.profile.devices, dst, deviceSrc);
    }
    state.profileEdited = true;
  },

  pushDevice(state, device: Hera.Device) {
    state.profile.devices.push(device);
    state.profileEdited = true;
  },

  setDeviceNameByIndex(state, payload: { deviceIndex: number; name: string }) {
    state.profile.devices[payload.deviceIndex].name = payload.name;
    state.profileEdited = true;
  },

  setDeviceForwardByIndex(state, payload: { deviceIndex: number; forward: boolean }) {
    state.profile.devices[payload.deviceIndex].forward = payload.forward;
    state.profileEdited = true;
  },

  setDeviceParameterByIndex(state, payload: { deviceIndex: number; parameterType: string; parameterValue: string }) {
    const foundIndex = state.profile.devices[payload.deviceIndex].parameters.findIndex(
      e => e.type == payload.parameterType
    );
    if (foundIndex >= 0) {
      state.profile.devices[payload.deviceIndex].parameters[foundIndex].value = payload.parameterValue;
    }
    state.profileEdited = true;
  }
};
