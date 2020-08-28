import { MutationTree } from 'vuex';
import { AcquisitionSettingState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<AcquisitionSettingState> = {
  setFromFetch(state, data: Hera.AcquisitionSetting) {
    state.fetchedData = data;
    state.isFetched = true;
    console.log(data);
  },

  // OperatorInfo
  setOperatorInfoOperatorName(state, data: string) {
    state.fetchedData.operatorInfo.operatorName = data;
  },
  setOperatorInfoPlace(state, data: string) {
    state.fetchedData.operatorInfo.place = data;
  },

  // Profile
  editNewProfile(state) {
    state.isProfileEditing = true;
    state.profileEditingIndex = -1;
  },
  editExistingProfile(state, index: number) {
    state.isProfileEditing = true;
    state.profileEditingIndex = index;
  },
  stopEditProfile(state) {
    state.isProfileEditing = false;
  }
};
