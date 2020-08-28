import { MutationTree } from 'vuex';
import { DeviceParameterState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<DeviceParameterState> = {
  setFromFetch(state, data: Array<Hera.DeviceAndParameters>) {
    state.fetchedData = data;
    console.log(data);
    state.isFetched = true;
  }
};
