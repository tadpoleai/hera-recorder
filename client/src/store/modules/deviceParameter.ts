import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Array<Hera.DeviceAndParameters>;
}

const state: State = {
  isFetched: false,
  fetchedData: []
};

const actions: ActionTree<State, RootState> = {
  async fetch({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getDeviceAndParameterses();
    commit('setFromFetch', data);
  },

  async adjustDeviceParameter({ commit, state, rootState, dispatch }, payload: { type: string; value: string }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.adjustDeviceParameter(
      rootState['DeviceData'].selectDetailDeviceIndex,
      payload.type,
      payload.value
    );
    commit('setFromFetch', data);
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state: State, data: Array<Hera.DeviceAndParameters>) {
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
