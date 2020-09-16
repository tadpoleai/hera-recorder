import { AcquisitionControlState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { connect } from '@/api';

export const actions: ActionTree<AcquisitionControlState, RootState> = {
  async fetch({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getStatus();
    commit('setFromFetch', data);
  },

  async start({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.start();
    commit('setFromFetch', data);
  },

  async stop({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.stop();
    commit('setFromFetch', data);
  },

  async setRecord({ commit }, record: boolean) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.setRecord(record);
    commit('setFromFetch', data);
  },

  clearError({ commit }) {
    commit('clearError');
  }
};
