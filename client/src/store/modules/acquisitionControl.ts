import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Hera.AcquisitionStatus;
  isErrorCleared: boolean;
}

const state: State = {
  isFetched: false,
  fetchedData: new Hera.AcquisitionStatus({
    error: 0,
    reason: '',
    started: false,
    recording: false,
    storageFileName: ''
  }),
  isErrorCleared: false
};

const actions: ActionTree<State, RootState> = {
  async fetch({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getStatus();
    commit('setFromFetch', data);
  },

  async start({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    // The Jetson may have no reliable clock source of its own (e.g. run as a WiFi
    // hotspot with no NTP uplink), so send the browser's own wall clock along --
    // the daemon self-corrects if its system clock has drifted far enough that
    // .hera/.insv filenames would otherwise carry a bogus timestamp.
    const data = await client.start(Date.now());
    commit('setFromFetch', data);
  },

  async stop({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.stop();
    commit('setFromFetch', data);
  },

  async setRecord({ commit, dispatch }, record: boolean) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.setRecord(record);
    commit('setFromFetch', data);
  },

  clearError({ commit }) {
    commit('clearError');
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state, data: Hera.AcquisitionStatus) {
    state.fetchedData = data;
    state.isFetched = true;
    state.isErrorCleared = false;
  },

  clearError(state, data: string) {
    state.isErrorCleared = true;
  }
};

const getters: GetterTree<State, RootState> = {
  started(state: State): boolean {
    if (!state.isFetched) {
      return false;
    } else {
      return state.fetchedData.started;
    }
  },

  immutable(state: State): boolean {
    if (!state.isFetched) {
      return true;
    } else {
      return state.fetchedData.started;
    }
  },

  recording(state: State): boolean {
    if (!state.isFetched) {
      return false;
    } else {
      return state.fetchedData.recording;
    }
  },

  isShowError(state: State): boolean {
    if (!state.isFetched) {
      return false;
    } else {
      return state.fetchedData.error != 0 && !state.isErrorCleared;
    }
  },

  errorReason(state: State): string {
    if (!state.isFetched) {
      return '';
    } else {
      return state.fetchedData.reason;
    }
  }
};

export const store: Module<State, RootState> = {
  namespaced: true,
  state,
  getters,
  actions,
  mutations
};

export default state;
