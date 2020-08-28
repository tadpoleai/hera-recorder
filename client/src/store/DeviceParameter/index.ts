import { Module } from 'vuex';
import { DeviceParameterState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: DeviceParameterState = {
  isFetched: false,
  fetchedData: []
};

const namespaced = true;
export const DeviceParameter: Module<DeviceParameterState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
