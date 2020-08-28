import { Vue } from 'vue-property-decorator';
import { MutationTree } from 'vuex';
import { MetaState, ParameterRule } from './types';

export const mutations: MutationTree<MetaState> = {
  addDeviceRule(state, payload: { name: string; rule: ParameterRule }) {
    Vue.set(state.deviceRulesMap, payload.name, payload.rule);
  }
};
