import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  profile: Hera.Profile;
  profileEdited: boolean;
}

const state: State = {
  profile: new Hera.Profile({
    name: '',
    author: '',
    devices: []
  }),
  profileEdited: false
};

const actions: ActionTree<State, RootState> = {
  setProfileName({ commit }, name: string) {
    commit('setProfileName', name);
  },

  setProfileAuthor({ commit }, author: string) {
    commit('setProfileAuthor', author);
  },

  addDeviceByType({ commit, rootState }, type: string) {
    const rules = rootState['Meta'].deviceDescriptionMap[type].parameters;
    const defaultParameters = rules.map(rule => {
      return new Hera.Parameter({ type: rule.name, value: rule.defaultValue });
    });
    commit(
      'pushDevice',
      new Hera.Device({
        type,
        name: '',
        forward: false,
        parameters: defaultParameters
      })
    );
  },

  deleteDeviceByIndex({ commit }, index: number) {
    commit('deleteDeviceByIndex', index);
  },

  moveDeviceIndex({ commit }, payload: { deviceIndex: number; direction: number }) {
    commit('moveDeviceIndex', payload);
  },

  setDeviceNameByIndex({ commit }, payload: { deviceIndex: number; name: string }) {
    commit('setDeviceNameByIndex', payload);
  },

  setDeviceForwardByIndex({ commit }, payload: { deviceIndex: number; forward: boolean }) {
    commit('setDeviceForwardByIndex', payload);
  },

  setDeviceParameterByIndex(
    { commit },
    payload: { deviceIndex: number; parameterType: string; parameterValue: string }
  ) {
    commit('setDeviceParameterByIndex', payload);
  }
};

const mutations: MutationTree<State> = {
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

const getters: GetterTree<State, RootState> = {
  profileValid(state): { valid: boolean; reason: string } {
    const ret = { valid: true, reason: '' };

    if (!RegExp('.+').test(state.profile.author)) {
      ret.reason += '创建人为空\n';
      ret.valid = false;
    }

    if (!RegExp('.+').test(state.profile.name)) {
      ret.reason += '配置名为空\n';
      ret.valid = false;
    }

    if (!state.profile.devices.length) {
      ret.reason += '传感器列表为空\n';
      ret.valid = false;
    }

    state.profile.devices.forEach((device, i) => {
      if (!RegExp('^[a-zA-Z0-9]+$').test(device.name)) {
        ret.reason += '第' + i + "个传感器名'" + device.name + "'不合法\n";
        ret.valid = false;
      }
    });

    return ret;
  }
};

export const store: Module<State, RootState> = {
  namespaced: true,
  state,
  getters,
  actions,
  mutations
};

export default state;
