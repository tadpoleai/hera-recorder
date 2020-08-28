import { MutationTree } from 'vuex';
import { DiskUsageState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<DiskUsageState> = {
  setFromFetch(state, data: Hera.DiskUsageStatus) {
    state.fetchedData = data;
    console.log(data);
    state.isFetched = true;
  }
};
