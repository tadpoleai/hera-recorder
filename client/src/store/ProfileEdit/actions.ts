import { ProfileEditState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export const actions: ActionTree<ProfileEditState, RootState> = {
  setProfileName({ commit }, name: string) {
    commit('setProfileName', name);
  },

  setProfileAuthor({ commit }, author: string) {
    commit('setProfileAuthor', author);
  },

  addDeviceByType({ commit, rootState }, type: string) {
    const rules = rootState['Meta'].deviceRulesMap[type];
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
