import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isConnectionErrored: boolean;
  connectionErrorReason: string;
  isErrorIgnored: boolean;

  hostUrl: string;
  hostPort: number;
}

const defaultHost = window.location.hostname;
const env = process.env.NODE_ENV || 'production';
let defaultPort = 10093;
if (env != 'production') {
  defaultPort = 10094;
}

const state: State = {
  isConnectionErrored: false,
  connectionErrorReason: '',
  isErrorIgnored: false,

  hostUrl: defaultHost,
  hostPort: defaultPort
};

const actions: ActionTree<State, RootState> = {
  refreshAll({ dispatch }) {
    dispatch('Meta/fetchMeta', null, { root: true });

    dispatch('AcquisitionControl/fetch', null, { root: true });
    dispatch('AcquisitionSetting/fetch', null, { root: true });

    dispatch('DiskUsage/fetch', null, { root: true });
  },

  refreshStatus({ dispatch }) {
    dispatch('AcquisitionControl/fetch', null, { root: true });
    dispatch('AcquisitionSetting/fetch', null, { root: true });

    dispatch('DiskUsage/fetch', null, { root: true });
  },

  onConnectionError({ commit }, reason: string) {
    commit('setConnectionError', reason);
  }
};

const mutations: MutationTree<State> = {
  setConnectionError(state, reason: string) {
    state.isConnectionErrored = true;
    state.connectionErrorReason = reason;
  },

  clearConnectionError(state) {
    state.isConnectionErrored = false;
    state.connectionErrorReason = '';
  },

  ignoreConnectionError(state) {
    state.isErrorIgnored = true;
  },

  clearErrorIgnored(state) {
    state.isErrorIgnored = false;
  },

  setHostUrl(state, value: string) {
    state.hostUrl = value;
  },

  setHostPort(state, value: number) {
    state.hostPort = value;
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
