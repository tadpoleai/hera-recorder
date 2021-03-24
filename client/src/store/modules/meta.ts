import Vue from 'vue';
import { Module, MutationTree, GetterTree, ActionTree } from 'vuex';
import { RootState } from '../types';

import ParameterRule from '@/interfaces/ParameterRule';
import DeviceDescription from '@/interfaces/DeviceDescription';

import { Hera, connect } from '@/api';

export interface State {
  isFetched: boolean;
  deviceDescriptionMap: Record<string, DeviceDescription>;
  daemonVersion: string;
}

const state: State = {
  isFetched: false,
  deviceDescriptionMap: {},
  daemonVersion: 'Unknown'
};

const actions: ActionTree<State, RootState> = {
  async fetchMeta({ commit, dispatch }) {
    const client = connect((err: any) => {
      dispatch('Main/onConnectionError', err.toString(), { root: true });
    });
    const meta = await client.getMeta();

    meta.deviceRules.forEach(deviceRule => {
      commit('addDeviceDescription', { name: deviceRule.name, description: JSON.parse(deviceRule.description) });
    });

    commit('setDaemonVersion', meta.daemonVersion);
  }
};

const mutations: MutationTree<State> = {
  addDeviceDescription(state, payload: { name: string; description: DeviceDescription }) {
    Vue.set(state.deviceDescriptionMap, payload.name, payload.description);
  },

  setDaemonVersion(state, value: string) {
    state.daemonVersion = value;
  }
};

const getters: GetterTree<State, RootState> = {
  deviceTypes(state: State): Array<string> {
    return Object.keys(state.deviceDescriptionMap);
  },

  categoriedDeviceTypes(state: State) {
    const ret: Array<{
      label: string;
      value: string;
      children: Array<{ value: string; label: string; comment: string }>;
    }> = [];
    for (const [deviceName, desc] of Object.entries(state.deviceDescriptionMap)) {
      const deviceNameWords = deviceName.split('/');
      const deviceLabelWords = desc.label.split('/');
      const deviceLabel =
        (deviceLabelWords.length > 1 ? deviceLabelWords[1] : desc.label) + '(' + deviceNameWords[1] + ')';

      const categoryName = deviceNameWords[0];
      const categoryLabel = deviceLabelWords[0] + '(' + categoryName + ')';

      const index = ret.findIndex(v => v.value == categoryName);

      const entry = { value: deviceName, label: deviceLabel, comment: desc.comment };

      if (index < 0) {
        ret.push({ label: categoryLabel, value: categoryName, children: [entry] });
      } else {
        ret[index].children.push(entry);
      }
    }
    return ret;
  },

  deviceParameterRuleMapMap(state: State): Record<string, Record<string, ParameterRule>> {
    const result: Record<string, Record<string, ParameterRule>> = {};
    for (const [deviceName, deviceDescription] of Object.entries(state.deviceDescriptionMap)) {
      result[deviceName] = {};
      deviceDescription.parameters.forEach(parameterRule => {
        result[deviceName][parameterRule.name] = parameterRule;
      });
    }
    return result;
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
