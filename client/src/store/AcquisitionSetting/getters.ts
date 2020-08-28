import { GetterTree } from 'vuex';
import { AcquisitionSettingState } from './types';
import { RootState } from '../types';

export const getters: GetterTree<AcquisitionSettingState, RootState> = {
  currentProfileName(state: AcquisitionSettingState): string {
    if (!state.isFetched) {
      return '[ Connection Lost ]';
    } else if (state.fetchedData.profiles.length == 0) {
      return '[ Empty ]';
    } else if (
      state.fetchedData.profileIndex < 0 ||
      state.fetchedData.profileIndex >= state.fetchedData.profiles.length
    ) {
      return '[ Index Error ]';
    } else {
      return state.fetchedData.profiles[state.fetchedData.profileIndex].name;
    }
  }
};
