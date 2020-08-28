import { Module } from 'vuex';
import { DeviceDataState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: DeviceDataState = {
  isFetched: false,
  fetchedData: new Hera.DataStatus({
    error: 0,
    reason: '',
    deviceDatas: [],
    slamResultValid: false,
    slamResult: new Buffer(''),
    startTimeSec: 0,
    nowTimeSec: 0
  }),
  isShowDetailDevice: false,
  selectDetailDeviceIndex: 0
};

const namespaced = true;
export const DeviceData: Module<DeviceDataState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
