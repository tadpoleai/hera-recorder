import { GetterTree } from 'vuex';
import { AcquisitionControlState } from './types';
import { RootState } from '../types';

export const getters: GetterTree<AcquisitionControlState, RootState> = {
  started(state: AcquisitionControlState): boolean {
    if (!state.isFetched) {
      return false;
    } else {
      return state.fetchedData.started;
    }
  },

  immutable(state: AcquisitionControlState): boolean {
    if (!state.isFetched) {
      return true;
    } else {
      return state.fetchedData.started;
    }
  },

  recording(state: AcquisitionControlState): boolean {
    if (!state.isFetched) {
      return false;
    } else {
      return state.fetchedData.recording;
    }
  },

  isShowError(state: AcquisitionControlState): boolean {
    if (!state.isFetched) {
      return false;
    } else {
      return state.fetchedData.error != 0 && !state.isErrorCleared;
    }
  },

  errorReason(state: AcquisitionControlState): string {
    if (!state.isFetched) {
      return '';
    } else {
      return state.fetchedData.reason;
    }
  }
};
