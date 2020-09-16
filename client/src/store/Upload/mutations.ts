import { MutationTree } from 'vuex';
import { UploadState } from './types';

import { Hera } from '@/api';

export const mutations: MutationTree<UploadState> = {
  setUploadServers(state, data: Array<string>) {
    state.uploadServers = data;
    state.isUploadServersFetched = true;
  },

  setUploadProcesses(state, data: Array<Hera.UploadProcess>) {
    state.uploadProcesses = data;
  }
};
