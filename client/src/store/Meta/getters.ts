import { GetterTree } from 'vuex';
import { MetaState, ParameterRule } from './types';
import { RootState } from '../types';

export const getters: GetterTree<MetaState, RootState> = {
  deviceTypes(state: MetaState): Array<string> {
    return Object.keys(state.deviceDescriptionMap);
  },

  categoriedDeviceTypes(state: MetaState) {
    const ret: Array<{
      text: string;
      value: string;
      children: Array<{ text: string; value: string; label: string; comment: string }>;
    }> = [];
    for (const [deviceName, desc] of Object.entries(state.deviceDescriptionMap)) {
      const deviceNameWords = deviceName.split('/');
      const categoryName = deviceNameWords[0];
      const subname = deviceNameWords[1];
      const index = ret.findIndex(v => v.value == categoryName);

      const entry = { text: subname, value: deviceName, label: desc.label, comment: desc.comment };

      if (index < 0) {
        ret.push({ text: categoryName, value: categoryName, children: [entry] });
      } else {
        ret[index].children.push(entry);
      }
    }
    return ret;
  },

  deviceParameterRuleMapMap(state: MetaState): Record<string, Record<string, ParameterRule>> {
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
