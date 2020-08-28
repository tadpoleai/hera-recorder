import { Module } from 'vuex';
import { AcquisitionControlState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: AcquisitionControlState = {
  isFetched: false,
  fetchedData: new Hera.AcquisitionStatus({
    error: 0,
    reason: '',
    started: false,
    recording: false,
    storageFileName: ''
  }),
  isErrorCleared: false
};

const namespaced = true;
export const AcquisitionControl: Module<AcquisitionControlState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
