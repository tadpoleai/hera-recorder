<template lang="pug">
div
  Parameter(
    v-for="parameterRule in activeParameterRules"
    :rule="parameterRule"
    :valueString="valuesMap[parameterRule.name]"
    :isAsync="isAdjustParameter"
    @asyncInput="asyncInput({type: parameterRule.name, value: $event})"
    @syncInput="syncInput({type: parameterRule.name, value: $event})"
  )

</template>

<script lang="ts">
import { Component, Emit, Vue, Prop } from 'vue-property-decorator';
import ParameterRule from '@/interfaces/ParameterRule';
import { namespace } from 'vuex-class';
import { Hera } from '@/api';

import Parameter from '@/components/Parameter.vue';
const MetaModule = namespace('Meta');

@Component({
  components: { Parameter }
})
export default class DetailData extends Vue {
  @Prop({ required: true }) private deviceType!: string;

  @Prop({ required: true }) private values!: Record<string, string> | Array<Hera.Parameter>;

  @Prop({ default: false, type: Boolean }) private isAdjustParameter!: boolean;

  @MetaModule.State deviceDescriptionMap;

  get activeParameterRules(): Array<ParameterRule> {
    const ret: Array<ParameterRule> = [];
    const parameterRules = this.deviceDescriptionMap[this.deviceType].parameters;

    parameterRules.forEach(parameterRule => {
      if (parameterRule.requirement.length == 0) {
        ret.push(parameterRule);
      } else {
        window['_HERA_PARAMETERS_EVAL_TMP_VALUES_MAP'] = this.valuesMap;
        window['_HERA_PARAMETERS_EVAL_TMP_IS_ADJUST'] = this.isAdjustParameter;
        const exp = parameterRule.requirement
          .replace(/#/g, 'window._HERA_PARAMETERS_EVAL_TMP_VALUES_MAP.')
          .replace(/$Started/g, 'window._HERA_PARAMETERS_EVAL_TMP_IS_ADJUST');
        try {
          const isActive = eval(exp);
          if (isActive) {
            ret.push(parameterRule);
          }
        } catch (e) {
          console.error(parameterRule, 'error when evaling', parameterRule.requirement, e);
          ret.push(parameterRule);
        }
      }
    });
    //  (not by others)
    // is mutable if in adjust mode
    return ret;
  }

  get valuesMap(): Record<string, string> {
    if (this.values instanceof Array) {
      const ret: Record<string, string> = {};
      (this.values as Array<Hera.Parameter>).forEach(element => {
        ret[element.type] = element.value;
      });
      return ret;
    } else {
      return this.values as Record<string, string>;
    }
  }

  @Emit('asyncInput') asyncInput(e: any) {
    return e;
  }

  @Emit('syncInput') syncInput(e: any) {
    return e;
  }
}
</script>

<style lang="stylus"></style>
