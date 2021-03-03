<template lang="pug">
.upload
  van-cell-group(title="数据文件名")
    van-cell
      template(v-for="file in filesToUpload")
        div(style="display: inline-block; margin: 4px")
          van-tag(plain round type="primary") {{ file }}
  
  template
    van-cell-group(title="服务器")
      van-radio-group(v-model="selectedServer")
        van-cell(
          v-for="serverName in uploadServers"
          icon="cluster-o"
          :title="serverName"
          clickable
          @click="onClickServer(serverName)"
        )
          van-radio(
            slot="right-icon"
            :name="serverName"
          )
  template
    van-cell-group(title="可移动储存")
      van-cell(
        v-for="disk in localDisks"
        icon="points"
        :title="disk.name"
        clickable
        @click="onClickLocalDisk(disk.name)"
      )
        template(slot="title")
          van-row
            van-col(span="5")
              span {{ disk.name }}
            van-col.right(span="19")
              span {{ dataSizeFormat(disk.diskUsageStatus.diskTotalSpace - disk.diskUsageStatus.diskUsedSpace) }} 空余
              span  / {{ dataSizeFormat(disk.diskUsageStatus.diskTotalSpace) }} 合计
          van-row(v-if="selectedLocalDiskPath.length && selectedLocalDiskPath[0] == disk.name")
            van-tag(plain type="primary" size="medium")
              | {{ selectedLocalDiskPathPlain }}
  van-cell()
    van-row(style="margin: 1em 0 0 0")
      van-col.center(span="12")
        van-button(
          type="danger"
          size="small"
          @click="$router.back()"
        ) 取消
      van-col.center(span="12")
        van-button(
          type="primary"
          size="small"
          @click="onClickConfirmUpload"
        ) 上传


  // Local Disk Browser
  van-overlay(
    :show="showLocalDiskBrowserDialog"
  )
    .wrapper
      .block(catch:tap="noop")
        van-cell-group(
          title="当前路径"
        )
          van-cell
            template(v-for="(layerName, index) in selectedLocalDiskPath")
              div(style="display: inline-block; margin: 4px")
                van-tag(
                  plain
                  size="large"
                  type="primary"
                  @click="onClickLayerIndex(index)"
                ) {{ layerName }}

        van-cell-group(
          title="子文件夹"
        )
        .sub-directories
          van-cell-group(
            v-show="!isFolderLoading"
            style="width: 100%"
          )
            van-cell(
              v-for="folder in selectedLocalDiskPathFolderContent"
              @click="onClickSubDirectory(folder)"
              style="width: 100%"
            )
              van-tag(plain) {{ folder }}

        van-cell()
          van-row(style="margin: 1em 0 0 0")
            van-col.center(span="24")
              van-button(
                type="info"
                size="small"
                @click="showLocalDiskBrowserDialog = false"
              ) 确定

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';
import { dataSizeFormatShort, dataSizeFormat } from '@/utils';

import DiskUsage from '@/components/DiskUsage.vue';
import { Toast } from 'vant';

const UploadModule = namespace('Upload');

@Component({
  components: { DiskUsage }
})
export default class Storage extends Vue {
  @UploadModule.State filesToUpload;

  @UploadModule.State uploadServers;
  @UploadModule.Action fetchUploadServers;
  @UploadModule.State localDisks;
  @UploadModule.Action fetchLocalDisks;

  @UploadModule.State selectedServer;
  @UploadModule.State selectedLocalDiskPath;

  @UploadModule.Mutation selectServer;
  @UploadModule.Action selectLocalDisk;
  @UploadModule.Action selectLocalSubDirectory;
  @UploadModule.Action selectLocalDirectoryIndex;

  @UploadModule.State isFolderLoading;
  @UploadModule.State selectedLocalDiskPathFolderContent;

  @UploadModule.Action startUpload;

  async created() {
    this.fetchUploadServers();
    this.fetchLocalDisks();
  }

  onClickServer(name: string) {
    this.selectServer(name);
  }

  onClickLocalDisk(name: string) {
    this.showLocalDiskBrowserDialog = true;
    this.selectLocalDisk(name);
  }

  onClickSubDirectory(name: string) {
    if (!this.isFolderLoading) {
      this.selectLocalSubDirectory(name);
    }
  }

  onClickLayerIndex(index: number) {
    if (!this.isFolderLoading) {
      this.selectLocalDirectoryIndex(index);
    }
  }

  async onClickConfirmUpload() {
    if (this.selectedServer == '' && this.selectedLocalDiskPath.length == 0) {
      Toast('没有选择上传目的地, 请选择服务器或本地储存器');
    } else {
      await this.startUpload();
      this.$router.back();
    }
  }

  get selectedLocalDiskPathPlain() {
    return '/ ' + this.selectedLocalDiskPath.slice(1).join(' / ');
  }

  showLocalDiskBrowserDialog = false;

  // Static methods

  dataSizeFormatShort = dataSizeFormatShort;

  dataSizeFormat = dataSizeFormat;
}
</script>

<style lang="stylus">
.center
  text-align center

.right
  text-align right

.wrapper
  display flex
  align-items center
  justify-content center
  height 100%

.block
  width 80vw
  height 80vh
  background-color #fff
  display flex
  flex-direction column

.sub-directories
  display flex
  flex 1
  overflow-y scroll
</style>
