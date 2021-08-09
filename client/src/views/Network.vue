<template lang="pug">
.network
  van-cell-group(title='网络适配器')
    template(v-for='base in fetchedData')
      van-cell(
        is-link,
        center,
        :title='base.ifName',
        :value='base.address',
        :label='base.netmask',
        @click='onClickBaseNetIf(base)'
      )
        span(slot='right-icon')

      van-swipe-cell.sub-net(v-for='(child, index) in base.children', :style='"background: " + colors[index % 8]')
        van-cell(
          is-link,
          center,
          :title='child.ifName',
          :value='child.address',
          :label='child.netmask',
          @click='onClickChildBaseNetIf(child)'
        )
          span(slot='right-icon')
        van-button.sub-net-button(
          slot='right',
          text='删除',
          type='danger',
          @click='onClickDeleteChildNetIf(child)',
          square
        )

  van-dialog(
    :title='dialogTitle',
    v-model:value='showDialog',
    :message='"OK"',
    theme='round-button',
    show-cancel-button,
    :show-confirm-button='subNetAddrValid && subNetAddrValid',
    cancelButtonText='取消',
    confirm-button-text='确定',
    message-align='center',
    closeOnClickOverlay,
    @confirm='onConfirmDialog'
  )
    van-field(label='地址(Addr)', v-model:value='subNetAddr', @input='checkSubNetAddr')
      CheckedIcon(slot='right-icon', :checked='subNetAddrValid')
    van-field(label='掩码(Mask)', v-model:value='subNetMask', @input='checkSubNetMask')
      CheckedIcon(slot='right-icon', :checked='subNetMaskValid')
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import CheckedIcon from '@/components/CheckedIcon.vue';
import { namespace } from 'vuex-class';
import { dataSizeFormatShort, dataSizeFormat } from '@/utils';
import { Hera } from '@/api';
import { Dialog } from 'vant';

const NetworkModule = namespace('Network');

@Component({
  components: { CheckedIcon }
})
export default class Network extends Vue {
  async created() {
    this.fetch();
  }

  @NetworkModule.State fetchedData;

  @NetworkModule.Action create;
  @NetworkModule.Action fetch;
  @NetworkModule.Action update;
  @NetworkModule.Action delete;

  async onClickBaseNetIf(base: Hera.NetworkInterface) {
    this.dialogTitle = '新增网络地址';
    this.showDialog = true;
    this.ifName = base.ifName;
    this.createMode = true;
    this.subNetAddrValid = false;
    this.subNetMaskValid = true;
    this.subNetAddr = base.address;
    this.subNetMask = '255.255.255.0';
  }

  checkSubNetAddr(value: string) {
    if (!this.ipReg.test(value)) {
      this.subNetAddrValid = false;
      return;
    }

    for (const i in this.fetchedData) {
      if (value == this.fetchedData[i].address) {
        this.subNetAddrValid = false;
        return;
      }
      for (const j in this.fetchedData[i].children) {
        if (value == this.fetchedData[i].children[j].address) {
          this.subNetAddrValid = false;
          return;
        }
      }
    }

    this.subNetAddrValid = true;
  }

  checkSubNetMask(value: string) {
    if (!this.netmaskReg.test(value)) {
      this.subNetMaskValid = false;
      return;
    }
    this.subNetMaskValid = true;
  }

  onCloseDialog() {
    this.showDialog = false;
  }

  async onClickDeleteChildNetIf(child: Hera.SubInterface) {
    try {
      await Dialog.confirm({
        title: '删除网络地址',
        message: "确认要网络地址 '" + child.ifName + "' 吗?",
        theme: 'round-button'
      });

      await this.delete({ name: child.ifName });
    } catch {
      return;
    }
  }

  onClickChildBaseNetIf(child: Hera.SubInterface) {
    this.dialogTitle = '修改网络地址';
    this.showDialog = true;
    this.ifName = child.ifName;
    this.createMode = false;
    this.subNetAddrValid = false;
    this.subNetMaskValid = true;
    this.subNetAddr = child.address;
    this.subNetMask = child.netmask;
  }

  onConfirmDialog() {
    if (this.createMode) {
      this.create({ name: this.ifName, address: this.subNetAddr, netmask: this.subNetMask });
    } else {
      this.update({ name: this.ifName, address: this.subNetAddr, netmask: this.subNetMask });
    }
  }

  createMode = true;

  ifName = '';

  subNetAddrValid = false;

  subNetAddr = '';

  subNetMaskValid = false;

  subNetMask = '';

  dialogTitle = '';

  showDialog = false;

  // Static

  dataSizeFormatShort = dataSizeFormatShort;

  dataSizeFormat = dataSizeFormat;

  colors = [
    'Aquamarine',
    'Chartreuse',
    'DarkOrange',
    'DeepSkyBlue',
    'GreenYellow',
    'LightPink',
    'OrangeRed',
    'Thistle'
  ];

  ipReg = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;

  netmaskReg = /^(((255\.){3}(255|254|252|248|240|224|192|128+))|((255\.){2}(255|254|252|248|240|224|192|128|0+)\.0)|((255\.)(255|254|252|248|240|224|192|128|0+)(\.0+){2})|((255|254|252|248|240|224|192|128|0+)(\.0+){3}))$/;
}
</script>

<style lang="stylus">
.sub-net {
  padding-left: 16px;
  background: lightblue;
}

.sub-net-button {
  height: 100%;
}
</style>
