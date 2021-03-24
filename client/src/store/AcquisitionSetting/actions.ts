import { AcquisitionSettingState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export const actions: ActionTree<AcquisitionSettingState, RootState> = {
  async fetch({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getSetting();
    commit('setFromFetch', data);
  },

  // OperatorInfo
  async setOperatorInfoSlam({ state, commit }, newValue: boolean) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    console.log(newValue);
    const data = await client.setOperatorInfo(
      new Hera.OperatorInfo({
        operatorName: state.fetchedData.operatorInfo.operatorName,
        place: state.fetchedData.operatorInfo.place,
        slam: newValue
      })
    );
    commit('setFromFetch', data);
  },

  setLocalOperatorInfoOperatorName({ commit }, newValue: string) {
    commit('setOperatorInfoOperatorName', newValue);
  },
  async updateOperatorInfoOperatorName({ commit, state }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.setOperatorInfo(state.fetchedData.operatorInfo);
    commit('setFromFetch', data);
  },

  setLocalOperatorInfoPlace({ commit }, newValue: string) {
    commit('setOperatorInfoPlace', newValue);
  },
  async updateOperatorInfoPlace({ commit, state }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.setOperatorInfo(state.fetchedData.operatorInfo);
    commit('setFromFetch', data);
  },

  // Profiles
  async selectProfile({ commit }, newIndex: number) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.selectProfile(newIndex);
    commit('setFromFetch', data);
  },

  async duplicateProfile({ commit, state }, index: number) {
    const newProfiles = state.fetchedData.profiles.slice();
    if (index < 0 || index >= newProfiles.length) {
      return;
    }

    const profileDuplicated = new Hera.Profile(Object.assign(newProfiles[index]));
    profileDuplicated.name += '_dup';
    newProfiles.splice(index + 1, 0, profileDuplicated);

    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.setProfiles(newProfiles);
    commit('setFromFetch', data);
  },

  async deleteProfile({ commit, state }, index: number) {
    const newProfiles = state.fetchedData.profiles.slice();
    newProfiles.splice(index, 1);

    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.setProfiles(newProfiles);
    commit('setFromFetch', data);
  },

  editNewProfile({ commit }) {
    commit('editNewProfile');
    commit('ProfileEdit/setProfile', new Hera.Profile({ name: '', author: '', devices: [] }), { root: true });
  },

  editExistingProfile({ commit, state }, index: number) {
    const existingProfile = state.fetchedData.profiles[index];
    commit('editExistingProfile', index);
    commit(
      'ProfileEdit/setProfile',
      new Hera.Profile({
        name: existingProfile.name,
        author: existingProfile.author,
        devices: existingProfile.devices.slice()
      }),
      { root: true }
    );
  },

  async finishEditingProfileAndSave({ commit, state, rootState }) {
    if (!state.isProfileEditing) {
      console.error('Not Profile Editing Mode');
      return;
    }

    const newProfiles = state.fetchedData.profiles.slice();
    const index = state.profileEditingIndex;
    if (index < 0 || index >= newProfiles.length) {
      console.log(rootState);
      console.log(rootState['ProfileEdit']['profile']);
      newProfiles.push(new Hera.Profile(rootState['ProfileEdit']['profile']));
    } else {
      newProfiles[index] = new Hera.Profile(rootState['ProfileEdit']['profile']);
    }

    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.setProfiles(newProfiles);
    commit('setFromFetch', data);
  }
};
