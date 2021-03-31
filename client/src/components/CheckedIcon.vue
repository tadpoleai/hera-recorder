<template lang="pug">
  van-icon.checked-icon(
    :name="iconName"
    size="large"
    :color="iconColor"
  )
</template>

<script lang="ts">
import { Component, Prop, Vue } from 'vue-property-decorator';

@Component({})
export default class CheckedIcon extends Vue {
  @Prop({ required: true }) private checked!: boolean;

  get iconName(): string {
    return this.checked ? 'passed' : 'warn-o';
  }

  get iconColor(): string {
    return this.checked ? 'green' : 'red';
  }
}
</script>

<style lang="stylus">
.checked-icon
  margin-left: 2px
</style>
