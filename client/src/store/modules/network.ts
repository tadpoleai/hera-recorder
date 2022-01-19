import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Array<Hera.NetworkInterface>;
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
    const data = await client.retrieveNetworkInterface();
    commit('setFromFetch', data);
  },

  async create({ commit, dispatch }, payload: { name: string; address: string; netmask: string }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });

    const data = await client.createNetworkInterface(payload.name, payload.address, payload.netmask);
    commit('setFromFetch', data);
  },

  async update({ commit, dispatch }, payload: { name: string; address: string; netmask: string }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });

    const data = await client.updateNetworkInterface(payload.name, payload.address, payload.netmask);
    commit('setFromFetch', data);
  },

  async delete({ commit, dispatch }, payload: { name: string }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });

    const data = await client.deleteNetworkInterface(payload.name);
    commit('setFromFetch', data);
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state, data: Array<Hera.NetworkInterface>) {
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
