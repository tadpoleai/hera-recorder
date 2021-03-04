import { ActionTree } from 'vuex';
import { RootState } from '../types';
import { MainState } from './types';

export const actions: ActionTree<MainState, RootState> = {
  refreshAll({ dispatch }) {
    dispatch('Meta/fetchMeta', null, { root: true });

    dispatch('AcquisitionControl/fetch', null, { root: true });
    dispatch('AcquisitionSetting/fetch', null, { root: true });

    dispatch('DiskUsage/fetch', null, { root: true });
  },

  refreshStatus({ dispatch }) {
    dispatch('AcquisitionControl/fetch', null, { root: true });
    dispatch('AcquisitionSetting/fetch', null, { root: true });

    dispatch('DiskUsage/fetch', null, { root: true });
  }
};
