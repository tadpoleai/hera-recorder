import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  fetchedData: Hera.AcquisitionSetting;
  isProfileEditing: boolean;
  profileEditingIndex: number;
}

const state: State = {
  isFetched: false,
  fetchedData: new Hera.AcquisitionSetting({
    profileIndex: -1,
    profiles: [],
    operatorInfo: new Hera.OperatorInfo({
      operatorName: '',
      place: '',
      slam: false
    })
  }),
  isProfileEditing: false,
  profileEditingIndex: -1
};

const actions: ActionTree<State, RootState> = {
  async fetch({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getSetting();
    commit('setFromFetch', data);
  },

  // OperatorInfo
  async setOperatorInfoSlam({ state, commit, dispatch }, newValue: boolean) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
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
  async updateOperatorInfoOperatorName({ commit, state, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.setOperatorInfo(state.fetchedData.operatorInfo);
    commit('setFromFetch', data);
  },

  setLocalOperatorInfoPlace({ commit }, newValue: string) {
    commit('setOperatorInfoPlace', newValue);
  },
  async updateOperatorInfoPlace({ commit, state, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.setOperatorInfo(state.fetchedData.operatorInfo);
    commit('setFromFetch', data);
  },

  // Profiles
  async selectProfile({ commit, dispatch }, newIndex: number) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.selectProfile(newIndex);
    commit('setFromFetch', data);
  },

  async duplicateProfile({ commit, state, dispatch }, index: number) {
    const newProfiles = state.fetchedData.profiles.slice();
    if (index < 0 || index >= newProfiles.length) {
      return;
    }

    const profileDuplicated = new Hera.Profile(Object.assign(newProfiles[index]));
    profileDuplicated.name += '_dup';
    newProfiles.splice(index + 1, 0, profileDuplicated);

    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.setProfiles(newProfiles);
    commit('setFromFetch', data);
  },

  async deleteProfile({ commit, state, dispatch }, index: number) {
    const newProfiles = state.fetchedData.profiles.slice();
    newProfiles.splice(index, 1);

    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
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

  async finishEditingProfileAndSave({ commit, state, rootState, dispatch }) {
    if (!state.isProfileEditing) {
      console.error('Not Profile Editing Mode');
      return;
    }

    const newProfiles = state.fetchedData.profiles.slice();
    const index = state.profileEditingIndex;
    if (index < 0 || index >= newProfiles.length) {
      newProfiles.push(new Hera.Profile(rootState['ProfileEdit']['profile']));
    } else {
      newProfiles[index] = new Hera.Profile(rootState['ProfileEdit']['profile']);
    }

    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.setProfiles(newProfiles);
    commit('setFromFetch', data);
    state.profileEditingIndex = state.fetchedData.profiles.length - 1;
  }
};

const mutations: MutationTree<State> = {
  setFromFetch(state, data: Hera.AcquisitionSetting) {
    state.fetchedData = data;
    state.isFetched = true;
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

const getters: GetterTree<State, RootState> = {
  currentProfileName(state: State): string {
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
  },

  indexValid(state: State): boolean {
    return state.fetchedData.profileIndex >= 0 && state.fetchedData.profileIndex < state.fetchedData.profiles.length;
  }
};

export const store: Module<State, RootState> = {
  namespaced: true,
  state,
  getters,
  actions,
  mutations
};

export default state;
