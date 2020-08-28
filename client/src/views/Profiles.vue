<template lang="pug">
div
  van-cell-group
    template(slot="title")
      div(style="display: flex; justify-content: space-between;")
        span 配置列表
        van-icon.title-right-icon.enabled-icon(
          name="add-o"
          @click="onClickAddProfile"
          size="24px"
        )

    van-radio-group(
      v-model="fetchedData.profileIndex"
    )
      template(
        v-if="fetchedData.profiles.length === 0"
      )
        van-empty(description="配置列表为空")
      //- For Every Profile
      van-swipe-cell(
        v-for="(profile, index) in fetchedData.profiles"
        :key="index"
      )
        van-cell(
          :title="profile.name"
          clickable
          @click="selectProfile(index)"
        )
          van-radio(
            slot="right-icon"
            :name="index"
          )
        van-button(
          slot="right"
          text="删除"
          type="danger"
          @click="deleteProfile(index)"
          square
        )
        template(slot="left")
          van-button(
            text="编辑"
            type="info"
            @click="onClickEditProfile(index)"
            square
          )
          van-button(
            text="复制"
            type="primary"
            @click="duplicateProfile(index)"
            square
          )
        
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';

const AcquisitionSettingModule = namespace('AcquisitionSetting');

@Component({})
export default class Profiles extends Vue {
  @AcquisitionSettingModule.State fetchedData;

  @AcquisitionSettingModule.Action selectProfile;

  @AcquisitionSettingModule.Action duplicateProfile;

  @AcquisitionSettingModule.Action deleteProfile;

  @AcquisitionSettingModule.Action editNewProfile;

  @AcquisitionSettingModule.Action editExistingProfile;

  onClickAddProfile() {
    this.editNewProfile();
    this.$router.push('profileedit');
  }

  onClickEditProfile(index: number) {
    this.editExistingProfile(index);
    this.$router.push('profileedit');
  }
}
</script>

<style lang="stylus">
.title-right-icon
  margin-left 4px
  margin-right 4px

.enabled-icon
  color #1989fa !important

.active-icon
  color red !important
</style>
