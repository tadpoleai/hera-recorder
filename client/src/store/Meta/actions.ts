import { ActionTree } from 'vuex';
import { RootState } from '../types';
import { MetaState } from './types';

import { Hera, connect } from '@/api';

export const actions: ActionTree<MetaState, RootState> = {
  async fetchMeta({ commit }) {
    const client = connect((err: any) => {
      commit('Main/setConnectionError', err.toString(), { root: true });
    });
    const meta = await client.getMeta();

    meta.deviceRules.forEach(deviceRule => {
      commit('addDeviceRule', { name: deviceRule.name, rule: JSON.parse(deviceRule.parameterRulesJson) });
    });
  }
};
