import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Hera.StorageStatus;
}

const state: State = {
  isFetched: false,
  fetchedData: new Hera.StorageStatus({ storageRecordFiles: [] })
};

const actions: ActionTree<State, RootState> = {
  async fetch({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getStorage();
    commit('setFromFetch', data);
  },

  async deleteStorage({ commit, dispatch }, names: Array<string>) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });

    const data = await client.deleteStorage(names);
    commit('setFromFetch', data);
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state, data: Hera.StorageStatus) {
    state.fetchedData = data;
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
