import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Hera.DataStatus;
  isShowDetailDevice: boolean;
  selectDetailDeviceIndex: number;
}

const state: State = {
  isFetched: false,
  fetchedData: new Hera.DataStatus({
    error: 0,
    reason: '',
    deviceDatas: [],
    slamResultValid: false,
    slamResult: new Buffer(''),
    startTimeSec: 0,
    nowTimeSec: 0
  }),
  isShowDetailDevice: false,
  selectDetailDeviceIndex: 0
};

const actions: ActionTree<State, RootState> = {
  async fetch({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getData();
    if (!data.error) {
      commit('setFromFetch', data);
    }
  },

  async clearSelectDetailDeviceIndex({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    commit('clearSelectDetailDeviceIndex');
    const data = await client.clearDetailDevice();
    commit('setFromFetch', data);
  },

  async setSelectDetailDeviceIndex({ commit, dispatch }, index: number) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    commit('setSelectDetailDeviceIndex', index);
    const data = await client.selectDetailDevice(index);
    commit('setFromFetch', data);
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state, data: Hera.DataStatus) {
    state.fetchedData = data;
    state.isFetched = true;
  },

  clearSelectDetailDeviceIndex(state) {
    state.isShowDetailDevice = false;
  },

  setSelectDetailDeviceIndex(state, data: number) {
    state.isShowDetailDevice = true;
    state.selectDetailDeviceIndex = data;
  }
};

const getters: GetterTree<State, RootState> = {
  selectedDetailData(state: State): Hera.DeviceData {
    return state.fetchedData.deviceDatas[state.selectDetailDeviceIndex];
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
