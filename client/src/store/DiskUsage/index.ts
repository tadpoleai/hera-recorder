import { Module } from 'vuex';
import { DiskUsageState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: DiskUsageState = {
  isFetched: false,
  fetchedData: new Hera.DiskUsageStatus({
    diskTotalSpaceKB: 0,
    diskUsedSpaceKB: 0
  })
};

const namespaced = true;
export const DiskUsage: Module<DiskUsageState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
