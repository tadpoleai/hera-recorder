import { Module } from 'vuex';
import { StorageState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: StorageState = {
  isFetched: false,
  fetchedData: new Hera.StorageStatus({ storageRecordFiles: [] })
};

const namespaced = true;
export const Storage: Module<StorageState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
