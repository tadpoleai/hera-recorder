import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Hera.DiskUsageStatus;
}

const state: State = {
  isFetched: false,
  fetchedData: new Hera.DiskUsageStatus({
    diskTotalSpace: 0,
    diskUsedSpace: 0
  })
};

const actions: ActionTree<State, RootState> = {
  async fetch({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getDiskUsageStatus();
    commit('setFromFetch', data);
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state, data: Hera.DiskUsageStatus) {
    state.fetchedData = data;
    console.log(data);
    state.isFetched = true;
  }
};

const getters: GetterTree<State, RootState> = {};

export const store: Module<State, RootState> = {
  namespaced: true,
  state,
  getters,
  actions,
  mutations
};

export default state;
