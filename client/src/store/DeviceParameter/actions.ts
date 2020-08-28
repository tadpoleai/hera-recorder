import { DeviceParameterState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export const actions: ActionTree<DeviceParameterState, RootState> = {
  async fetch({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getDeviceAndParameterses();
    commit('setFromFetch', data);
  },

  async adjustDeviceParameter({ commit, state, rootState }, payload: { type: string; value: string }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.adjustDeviceParameter(rootState['DeviceData'].selectDetailDeviceIndex, payload.type, payload.value);
    commit('setFromFetch', data);
  }
};
