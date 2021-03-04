<template lang="pug">
van-cell(
  :title="rule.label"
  center
)
  template(
    slot="icon"
  )
    van-icon.info-button(
      name="question-o"
      size="24px"
      color="#1989fa"
      @click="showInformation"
    )

  //- Boolean
  template(
    v-if="rule.type == 'boolean'"
  )
    van-switch(
      slot="right-icon"
      :value="value"
      @input="setValueSync"
      size="24px"
    )
  //- Numeric
  template(
    v-else-if="rule.type == 'double' || rule.type == 'int'"
  )
    van-stepper.numeric_input(
      :value="value"
      @input="setValue"
      :integer="rule.type == 'int'"
      :max="rule.range.max"
      :min="rule.range.min"
      :step="rule.range.step"
      theme="round"
      :input-width="'55%'"
      button-size="24px"
    )
    van-icon.async-confirm-button(
      v-if="isAsync"
      v-bind:class="{hidden: !modified}"
      slot="right-icon"
      name="upgrade"
      :color="modified? 'red' : 'gray'"
      type="primary"
      size="24px"
      @click="onClickUpdate"
    )

  //- Enum
  template(
    v-else-if="rule.type == 'enum'"
  )
    van-dropdown-menu(
      direction="up"
    )
      van-dropdown-item(
        :value="value"
        @input="setValueSync"
        :options="getOptionsVant"
      )
  //- String
  template(
    v-else-if="rule.type == 'string'"
  )
    CheckedField.string_input(
      :value="value"
      @input="setValue"
      :regex="rule.regex"
      :placeholder="rule.defaultValue"
      input-align="right"
      style="padding: 0 !important;"
    )
    van-icon.async-confirm-button(
      v-if="isAsync"
      slot="right-icon"
      name="upgrade"
      :color="modified? 'red' : 'gray'"
      type="primary"
      size="24px"
      @click="onClickUpdate"
    )
</template>

<script lang="ts">
import { Component, Emit, Prop, Vue } from 'vue-property-decorator';
import { ParameterRule } from '../store/Meta/types';
import { Dialog } from 'vant';

import CheckedField from '@/components/CheckedField.vue';
@Component({
  components: { CheckedField }
})
export default class Parameter extends Vue {
  @Prop({ required: true }) private valueString!: string;

  @Prop({ required: true }) private rule!: ParameterRule;

  @Prop({ default: false, type: Boolean }) private isAsync!: boolean;

  localValue!: string;

  modified = false;

  created() {
    this.localValue = this.valueString;
  }

  beforeDestory() {
    if (this.modified) {
      this.setAsyncValue();
    }
  }

  get value(): string | number | boolean {
    if (this.rule.type == 'boolean') {
      return this.valueString == 'true';
    } else if (this.rule.type == 'double') {
      return parseFloat(this.valueString);
    } else if (this.rule.type == 'int') {
      return parseInt(this.valueString);
    } else {
      return this.valueString;
    }
  }

  get getOptionsVant(): Array<{ text: string; value: string }> {
    return this.rule.options!.map(option => {
      return { text: option, value: option };
    });
  }

  setValue(newValue: string | number | boolean): void {
    if (!this.isAsync) {
      this.setSyncValue(newValue.toString());
    } else {
      this.modified = true;
      this.localValue = newValue.toString();
    }
  }

  setValueSync(newValue: string | number | boolean): void {
    if (!this.isAsync) {
      this.setSyncValue(newValue.toString());
    } else {
      this.localValue = newValue.toString();
      this.setAsyncValue();
    }
  }

  @Emit('syncInput') setSyncValue(newValue: string): string {
    return newValue;
  }

  onClickUpdate() {
    if (this.modified) {
      this.setAsyncValue();
    }
  }

  @Emit('asyncInput') setAsyncValue(): string {
    this.modified = false;
    console.log(this.localValue);
    return this.localValue;
  }

  showInformation() {
    Dialog({
      title: this.rule.label,
      message: this.rule.comment,
      messageAlign: 'left',
      theme: 'round-button',
      width: '95vw'
    });
  }
}
</script>

<style lang="stylus">
.info-button
  margin-right: 2px

.checked-button
  margin-left: 12px
  margin-right: 0px

.async-confirm-button
  margin-left: 12px
  margin-right: 0px

.van-dropdown-menu__title
  margin-left: auto
  margin-right: 8px

.van-dropdown-menu__bar
  height: 20px !important

.van-dropdown-item__content
  padding-bottom: 12px

.van-dropdown-item__option
  padding-left: 48px !important
  padding-right: 32px !important
</style>
