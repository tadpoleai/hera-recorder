import { Module } from 'vuex';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';
import { MetaState } from './types';

const state: MetaState = {
  isFetched: false,
  deviceRulesMap: {}
};

const namespaced = true;
export const Meta: Module<MetaState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
