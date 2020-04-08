<template lang="pug">
div
  van-nav-bar(title="选择配置" left-arrow @click-left="clickNavBack()")

  van-cell-group
    template(slot="title")
      div(style="display: flex; justify-content: space-between;")
        span 采集配置列表
        van-icon(type="info" name="more-o" size="large" @click="clickAddProfile()")

    van-radio-group(v-model="status.local.currentProfileIndex")
      template(v-if="status.local.profiles.length === 0")
        van-cell(title="列表为空")
      //- For Every Profile
      van-swipe-cell(
        v-for="(profile, index) in status.local.profiles"
        :key="index")
        van-cell(:title="profile.name" clickable @click="clickSelectProfile(index)")
          van-radio(slot="right-icon" :name="index")
        template(slot="right")
          van-button(square type="danger" text="删除" @click="clickDeleteProfile(index)")
        template(slot="left")
          van-button(square type="primary" text="编辑" @click="clickEditProfile(index)")
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';

console.log(status);

@Component({})
export default class Profile extends Vue {
  clickNavBack() {
    this.$router.back();
  }

  clickAddProfile() {
    this.status.local.profileToEdit = new Hera.Profile({
      author: '',
      name: '',
      devices: []
    });
    this.status.local.profileAddOrEdit = true;
    this.$router.push('profile/edit');
  }

  clickEditProfile(index: number) {
    this.status.local.profileToEdit = this.status.local.profiles[index];
    this.status.local.profileAddOrEdit = false;
    this.$router.push('profile/edit');
  }

  clickDeleteProfile(index: number) {
    this.status.local.profiles.splice(index, 1);
    Api.updateProfiles();
  }

  clickSelectProfile(index: number) {
    this.status.local.currentProfileIndex = index;
  }

  status = status;
}
</script>
