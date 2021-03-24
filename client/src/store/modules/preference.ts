import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  useFab: boolean;
}

const state: State = {
  useFab: true
};

const booleanKeys = ['useFab'];
for (const index in booleanKeys) {
  const key = booleanKeys[index];
  const ret = localStorage.getItem(key);
  if (ret) {
    state[key] = ret == 'true';
  } else {
    localStorage.setItem(key, state[key]);
  }
}

const actions: ActionTree<State, RootState> = {};

const mutations: MutationTree<State> = {
  setPreference(state, payload: { key: string; value: any }) {
    state[payload.key] = payload.value;
    localStorage[payload.key] = payload.value;
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
