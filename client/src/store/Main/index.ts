import { Module } from 'vuex';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { MainState } from './types';

const state: MainState = {
  isConnectionErrored: false,
  connectionErrorReason: ''
};

const namespaced = true;
export const Main: Module<MainState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
