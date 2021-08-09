<template lang="pug">
van-cell
  template(slot='title')
    van-tag(:plain='source.level == 0x10', :type='toTagType(source.level)') {{ timestampFormat(source.tsSec, source.tsNsec) }}
  template
    span(v-if='isSingleLine(source.message)') {{ source.message }}
    template(v-else)
      p.p-log(v-for='(p, i) in renderStringData(source.message)') {{ p }}
</template>

<script lang="ts">
import { Component, Vue, Prop } from 'vue-property-decorator';

import { Hera } from '@/api';
import { timestampFormat } from '@/utils';

@Component({
  components: {}
})
export default class ListItem extends Vue {
  @Prop({ required: true }) private index!: number;

  @Prop({ required: true }) private source!: Hera.LogMessage;

  toTagType(level: number) {
    switch (level) {
      case 0x10:
        return 'primary';
      case 0x11:
        return 'primary';
      case 0x12:
        return 'warning';
      case 0x13:
        return 'danger';
      default:
        return 'danger';
    }
  }

  isSingleLine(str: string) {
    return str.indexOf('\n') == -1;
  }

  renderStringData(str: string) {
    return str.split('\n');
  }

  timestampFormat = timestampFormat;
}
</script>

<style lang="stylus">
.p-log {
  text-align: left;
}
</style>
