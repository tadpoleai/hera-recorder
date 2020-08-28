import { GetterTree } from 'vuex';
import { MetaState, ParameterRule } from './types';
import { RootState } from '../types';

export const getters: GetterTree<MetaState, RootState> = {
  deviceTypes(state: MetaState): Array<string> {
    return Object.keys(state.deviceRulesMap);
  },

  deviceParameterRuleMapMap(state: MetaState): Record<string, Record<string, ParameterRule>> {
    const result: Record<string, Record<string, ParameterRule>> = {};
    for (const [deviceName, parameterRules] of Object.entries(state.deviceRulesMap)) {
      result[deviceName] = {};
      parameterRules.forEach(parameterRule => {
        result[deviceName][parameterRule.name] = parameterRule;
      });
    }
    return result;
  }
};
