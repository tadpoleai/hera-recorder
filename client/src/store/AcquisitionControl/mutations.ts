import { MutationTree } from 'vuex';
import { AcquisitionControlState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<AcquisitionControlState> = {
  setFromFetch(state, data: Hera.AcquisitionStatus) {
    state.fetchedData = data;
    state.isFetched = true;
    state.isErrorCleared = false;
  },

  clearError(state, data: string) {
    state.isErrorCleared = true;
  }
};
