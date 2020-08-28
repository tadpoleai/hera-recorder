import { Module } from 'vuex';
import { ProfileEditState } from './types';
import { RootState } from '../types';
import { getters } from './getters';
import { actions } from './actions';
import { mutations } from './mutations';

import { Hera } from '@/api';

const state: ProfileEditState = {
  profile: new Hera.Profile({
    name: '',
    author: '',
    devices: []
  }),
  profileEdited: false
};

const namespaced = true;
export const ProfileEdit: Module<ProfileEditState, RootState> = {
  namespaced,
  state,
  getters,
  actions,
  mutations
};
export default state;
