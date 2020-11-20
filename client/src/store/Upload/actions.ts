import { UploadState } from './types';
import { ActionTree } from 'vuex';
import { RootState } from '../types';

import { Hera, connect } from '@/api';
import { Server } from 'http';
import { UploadOperationType } from '@/api/gen-ts';

export const actions: ActionTree<UploadState, RootState> = {
  async fetchUploadServers({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getUploadServers();
    commit('setUploadServers', data);
  },

  async fetchUploadProcesses({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.getUploadProcesses();
    commit('setUploadProcesses', data);
  },

  async startUpload({ commit }, payload: { name: string; remote: string }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload(
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: UploadOperationType.Start,
        compress: false
      })
    );
    commit('setUploadProcesses', data);
  },

  async completeUpload({ commit }, payload: Hera.UploadRequest) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload(
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: UploadOperationType.Complete,
        compress: false
      })
    );
    commit('setUploadProcesses', data);
  },

  async retryUpload({ commit }, payload: Hera.UploadRequest) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload(
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: UploadOperationType.Retry,
        compress: false
      })
    );
    commit('setUploadProcesses', data);
  },

  async abortUpload({ commit }, payload: Hera.UploadRequest) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const data = await client.requestUpload(
      new Hera.UploadRequest({
        name: payload.name,
        remote: payload.remote,
        operationType: UploadOperationType.Abort,
        compress: false
      })
    );
    commit('setUploadProcesses', data);
  }
};
