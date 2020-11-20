import { MutationTree } from 'vuex';
import { StorageState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<StorageState> = {
  setFromFetch(state, data: Hera.StorageStatus) {
    state.fetchedData = data;
    console.log(data);
    state.isFetched = true;
  }
};
