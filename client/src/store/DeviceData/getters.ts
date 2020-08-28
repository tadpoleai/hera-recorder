import { GetterTree } from 'vuex';
import { DeviceDataState } from './types';
import { RootState } from '../types';
import { Hera } from '@/api';

export const getters: GetterTree<DeviceDataState, RootState> = {
  selectedDetailData(state: DeviceDataState): Hera.DeviceData {
    return state.fetchedData.deviceDatas[state.selectDetailDeviceIndex];
  }
};
