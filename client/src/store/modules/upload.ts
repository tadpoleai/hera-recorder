import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';

export interface State {
  uploadProcesses: Array<Hera.UploadProcess>;

  filesToUpload: Array<string>;

  uploadServers: Array<string>;
  localDisks: Array<Hera.LocalDisk>;

  selectedServer: string;
  isFolderLoading: boolean;
  selectedLocalDiskPath: Array<string>;
  selectedLocalDiskPathFolderContent: Array<string>;
}

const state: State = {
  uploadProcesses: [],

  filesToUpload: [],

  uploadServers: [],
  localDisks: [],

  selectedServer: '',
  isFolderLoading: false,
  selectedLocalDiskPath: [],
  selectedLocalDiskPathFolderContent: []
};

const actions: ActionTree<State, RootState> = {
  async fetchUploadServers({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getUploadServers();
    commit('setUploadServers', data);
  },

  async fetchLocalDisks({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getLocalDisks();
    commit('setLocalDisks', data);
  },

  async selectLocalDisk({ commit, dispatch, state }, name: string) {
    commit('selectLocalDisk', name);

    dispatch('fetchLocalDiskFolders');
  },

  async selectLocalSubDirectory({ commit, dispatch, state }, name: string) {
    commit('selectLocalSubDirectory', name);

    dispatch('fetchLocalDiskFolders');
  },

  async selectLocalDirectoryIndex({ commit, dispatch, state }, index: number) {
    commit('selectLocalDirectoryIndex', index);

    dispatch('fetchLocalDiskFolders');
  },

  async unSelectLocalDisk({ commit, dispatch, state }) {
    commit('unSelectLocalDisk');
  },

  async fetchLocalDiskFolders({ commit, dispatch, state }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });

    commit('setIsFolderLoading', true);

    const data = await client.getLocalDiskFolders(state.selectedLocalDiskPath.slice());
    console.log(data);

    commit('setSelectedLocalDiskPathFolderContent', data);
    commit('setIsFolderLoading', false);
  },

  async fetchUploadProcesses({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.getUploadProcesses();
    commit('setUploadProcesses', data);
  },

  async startUpload({ commit, dispatch, state }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });

    const param = [] as Array<Hera.UploadRequest>;
    if (state.selectedServer != '') {
      for (const i in state.filesToUpload) {
        const fileName = state.filesToUpload[i];

        param.push(
          new Hera.UploadRequest({
            name: fileName,
            remote: state.selectedServer,
            operationType: Hera.UploadOperationType.Start,
            extraPath: '',
            compress: false
          })
        );
      }
    } else {
      for (const i in state.filesToUpload) {
        const fileName = state.filesToUpload[i];

        param.push(
          new Hera.UploadRequest({
            name: fileName,
            remote: '',
            operationType: Hera.UploadOperationType.Start,
            extraPath: state.selectedLocalDiskPath.join('/'),
            compress: false
          })
        );
      }
    }

    const data = await client.requestUpload(param);

    commit('setUploadProcesses', data);
  },

  async completeUpload({ commit, dispatch }, payload: Hera.UploadRequest) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload([
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: Hera.UploadOperationType.Complete,
        extraPath: payload.extraPath,
        compress: false
      })
    ]);
    commit('setUploadProcesses', data);
  },

  async retryUpload({ commit, dispatch }, payload: Hera.UploadRequest) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload([
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: Hera.UploadOperationType.Retry,
        extraPath: payload.extraPath,
        compress: false
      })
    ]);
    commit('setUploadProcesses', data);
  },

  async abortUpload({ commit, dispatch }, payload: Hera.UploadRequest) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload([
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: Hera.UploadOperationType.Abort,
        extraPath: payload.extraPath,
        compress: false
      })
    ]);
    commit('setUploadProcesses', data);
  }
};

const mutations: MutationTree<State> = {
  setUploadServers(state, data: Array<string>) {
    state.uploadServers = data;
  },

  setLocalDisks(state, data: Array<Hera.LocalDisk>) {
    state.localDisks = data;
  },

  setUploadProcesses(state, data: Array<Hera.UploadProcess>) {
    state.uploadProcesses = data;
  },

  setFilesToUpload(state, data: Array<string>) {
    state.filesToUpload = data;
  },

  selectLocalDisk(state, name: string) {
    state.selectedServer = '';
    if (state.selectedLocalDiskPath.length == 0 || state.selectedLocalDiskPath[0] != name) {
      state.selectedLocalDiskPath = [name];
    }
  },

  selectLocalSubDirectory(state, name: string) {
    state.selectedLocalDiskPath.push(name);
  },

  selectLocalDirectoryIndex(state, index: number) {
    state.selectedLocalDiskPath = state.selectedLocalDiskPath.slice(0, index + 1);
  },

  selectServer(state, name: string) {
    state.selectedServer = name;
    state.selectedLocalDiskPath = [];
    state.selectedLocalDiskPathFolderContent = [];
  },

  setIsFolderLoading(state, value: boolean) {
    state.isFolderLoading = value;
  },

  setSelectedLocalDiskPathFolderContent(state, data: Array<string>) {
    state.selectedLocalDiskPathFolderContent = data;
  }
};

const getters: GetterTree<State, RootState> = {
  isUploadProcessesActive(state: State): boolean {
    return state.uploadProcesses.some(p => p.running);
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
