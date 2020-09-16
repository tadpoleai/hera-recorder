import { Module } from 'vuex';
import { UploadState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: UploadState = {
  isUploadServersFetched: false,
  uploadServers: [],
  uploadProcesses: []
};

const namespaced = true;
export const Upload: Module<UploadState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
