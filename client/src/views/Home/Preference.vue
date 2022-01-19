<template lang="pug">
van-cell-group(title='其他设置')
  van-cell(key='1', title='快捷操作按钮', center)
    van-switch(
      slot='right-icon',
      :value='useFab',
      @input='setPreference({ key: "useFab", value: $event })',
      size='24px'
    )
  van-cell(key='2', title='网络设置', is-link, @click='onClickNetwork')
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';

const PreferenceModule = namespace('Preference');

@Component({
  components: {}
})
export default class Preference extends Vue {
  @PreferenceModule.State useFab;

  @PreferenceModule.Mutation setPreference;

  show = false;

  onClickNetwork() {
    this.$router.push('network');
  }
}
</script>

<style lang="stylus"></style>
