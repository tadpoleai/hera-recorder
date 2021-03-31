<template lang="pug">
van-cell-group
  div(slot="title" style="display: flex; justify-content: space-between;")
    span 偏好设置
    van-icon(
      type="info"
      :name="show ? 'arrow-up' : 'arrow-down'"
      size="large"
      @click="show = !show"
    )
  transition(name="van-fade")
    template(
      v-if="show"
    )
      van-cell(
        title="快捷操作按钮"
        center
      )
        van-switch(
          slot="right-icon"
          :value="useFab"
          @input="setPreference({ key: 'useFab', value: $event })"
          size="24px"
        )
 
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
}
</script>

<style lang="stylus"></style>
