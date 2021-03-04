import { MutationTree } from 'vuex';
import { MainState } from './types';

export const mutations: MutationTree<MainState> = {
  setConnectionError(state, reason: string) {
    state.isConnectionErrored = true;
    state.connectionErrorReason = reason;
    console.log(reason);
  },

  clearConnectionError(state) {
    state.isConnectionErrored = false;
    state.connectionErrorReason = '';
  },

  setHostUrl(state, value: string) {
    state.hostUrl = value;
  },

  setHostPort(state, value: number) {
    state.hostPort = value;
  }
};
