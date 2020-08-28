import { Module } from 'vuex';
import { AcquisitionSettingState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: AcquisitionSettingState = {
  isFetched: false,
  fetchedData: new Hera.AcquisitionSetting({
    profileIndex: -1,
    profiles: [],
    operatorInfo: new Hera.OperatorInfo({
      operatorName: '',
      place: '',
      slam: false
    })
  }),
  isProfileEditing: false,
  profileEditingIndex: -1,
};

const namespaced = true;
export const AcquisitionSetting: Module<AcquisitionSettingState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
