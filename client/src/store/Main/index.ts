import { Module } from 'vuex';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { MainState } from './types';

const defaultHost = window.location.hostname;
const env = process.env.NODE_ENV || 'production';
let defaultPort = 9090;
if (env != 'production') {
  defaultPort = 9091;
}

const state: MainState = {
  isConnectionErrored: false,
  connectionErrorReason: '',

  hostUrl: defaultHost,
  hostPort: defaultPort
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
