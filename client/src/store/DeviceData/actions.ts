import { DeviceDataState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export const actions: ActionTree<DeviceDataState, RootState> = {
  async fetch({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getData();
    if (!data.error) {
      commit('setFromFetch', data);
    }
  },

  async clearSelectDetailDeviceIndex({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    commit('clearSelectDetailDeviceIndex');
    const data = await client.clearDetailDevice();
    commit('setFromFetch', data);
  },

  async setSelectDetailDeviceIndex({ commit }, index: number) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    commit('setSelectDetailDeviceIndex', index);
    const data = await client.selectDetailDevice(index);
    commit('setFromFetch', data);
  }
};
