import { GetterTree } from 'vuex';
import { UploadState } from './types';
import { RootState } from '../types';
import { Hera } from '@/api';

export const getters: GetterTree<UploadState, RootState> = {
  isUploadProcessesActive(state: UploadState): boolean {
    return state.uploadProcesses.some(p => p.running);
  }
};
