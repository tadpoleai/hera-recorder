import { StorageState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export const actions: ActionTree<StorageState, RootState> = {
  async fetch({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getStorage();
    commit('setFromFetch', data);
  },

  async deleteStorage({ commit }, name: string) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.deleteStorage(name);
    commit('setFromFetch', data);
  }
};
