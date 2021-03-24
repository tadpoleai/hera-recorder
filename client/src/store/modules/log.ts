import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';
import { LogMessage } from '@/api/gen-ts';

export interface State {
  messages: Array<Hera.LogMessage>;
  loading: boolean;
}

const state: State = {
  messages: [],
  loading: false
};

const actions: ActionTree<State, RootState> = {
  async syncLog({ commit, state }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });

    const data = await client.latestLogs();
    commit('syncLog', data);
  }
};

const mutations: MutationTree<State> = {
  syncLog(state: State, data: Array<LogMessage>) {
    state.messages = data;
  }
};

const getters: GetterTree<State, RootState> = {
  //
};

export const store: Module<State, RootState> = {
  namespaced: true,
  state,
  getters,
  actions,
  mutations
};

export default state;
