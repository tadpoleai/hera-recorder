<template lang="pug">
van-field(:value='value', @input='input', @blur='change', v-bind='$attrs')
  CheckedIcon(slot='right-icon', :checked='checked')
</template>

<script lang="ts">
import { Component, Emit, Prop, Vue, Watch } from 'vue-property-decorator';

import CheckedIcon from '@/components/CheckedIcon.vue';
@Component({
  components: { CheckedIcon }
})
export default class CheckField extends Vue {
  @Prop({ required: true }) private regex!: string;

  @Prop({ required: true }) private value!: string;

  get regExp(): RegExp {
    return new RegExp(this.regex);
  }

  get checked(): boolean {
    return this.regExp.test(this.value);
  }

  @Emit('input') input(value: string): string {
    return value;
  }

  @Emit('change') change(): string {
    return this.value;
  }
}
</script>

<style lang="stylus">
.checked-icon {
  margin-left: 4px;
}
</style>
