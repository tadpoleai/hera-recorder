import { MutationTree } from 'vuex';
import { DeviceDataState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<DeviceDataState> = {
  setFromFetch(state, data: Hera.DataStatus) {
    state.fetchedData = data;
    state.isFetched = true;
  },

  clearSelectDetailDeviceIndex(state) {
    state.isShowDetailDevice = false;
  },

  setSelectDetailDeviceIndex(state, data: number) {
    state.isShowDetailDevice = true;
    state.selectDetailDeviceIndex = data;
  }
};
