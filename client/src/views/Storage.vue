<template lang="pug">
.storage
  transition(
    name="van-fade"
  )
    van-cell-group(
      v-if="uploadProcesses.length"
      title="上传进度"
    )
      transition(
        name="van-fade"
        v-for="p in uploadProcesses"
      )
        UploadProcess(:process="p")

  van-cell-group
    div(slot="title" style="display: flex; justify-content: space-between;")
      span 本地数据
      van-icon(type="info" name="replay" size="large" @click="onClickUpdate()")

    DiskUsage(
      :refresh="true"
    ) 

    van-collapse(
      v-model="activeName"
      accordion
    )
      //- For storage record files
      van-collapse-item(
        v-for="(storage, index) in fetchedData"
        :title="storage.name"
      )
        div(slot="title" style="display: flex; justify-content: space-between;")
          span {{storage.name}}
          span {{dataSizeFormatShort(storage.sizeKB)}}
        van-cell
          van-row()
            van-col(span="12") 开始时间
            van-col.right(span="12") 结束时间
          van-row()
            van-col(span="12") {{storage.startTime}}
            van-col.right(span="12") {{storage.endTime}}
        van-cell
          van-row
            van-col(span="12") 传感器名称
            van-col.right(span="5") 消息数量
            van-col.right(span="7") 占用容量

          van-row(v-for="d in storage.devices")
            van-col(span="13") {{d.typeName}}
            van-col.right(span="4") {{d.messageNum}}
            van-col.right(span="7") {{dataSizeFormatShort(d.dataSizeKB)}}
        van-cell
          van-row(style="margin: 1em 0 0 0")
            van-col.center(span="12")
              van-button(
                type="danger"
                size="small"
                @click="onClickDelete(storage.name)"
              ) 删除
            van-col.center(span="12")
              van-button(
                type="primary"
                size="small"
                @click="onClickUpload(storage.name)"
              ) 上传

  //- Dialog of Upload Request
  van-dialog(
    v-model="showUploadDialog"
    title="上传数据"
    show-cancel-button
    show-confirm-button
    :before-close="onConfirmUpload"
  )
    van-cell-group(title="数据名称")
      van-cell(:title="recordFileNameToUpload")
    van-radio-group(v-model="selectedUploadServer")
      van-cell-group(title="服务器")
        template(v-if="uploadServers.length > 0")
          van-cell(
            v-for="serverName in uploadServers"
            :title="serverName"
            clickable
            @click="selectedUploadServer = serverName"
          )
            van-radio(
              slot="right-icon"
              :name="serverName"
            )
        template(v-else)
          van-cell 没有可用服务器, 请联系管理员
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { State, Getter, Action, Mutation, namespace } from 'vuex-class';
import { Toast, Dialog } from 'vant';
import { dataSizeFormatShort } from '@/utils';

import DiskUsage from '@/components/DiskUsage.vue';
import UploadProcess from '@/views/Storage/UploadProcess.vue';

const StorageModule = namespace('Storage');
const UploadModule = namespace('Upload');

@Component({
  components: { DiskUsage, UploadProcess }
})
export default class Storage extends Vue {
  @StorageModule.State fetchedData;

  @StorageModule.Action fetch;
  @StorageModule.Action deleteStorage;

  @UploadModule.State uploadServers;
  @UploadModule.Action fetchUploadServers;

  @UploadModule.State uploadProcesses;
  @UploadModule.Action fetchUploadProcesses;
  @UploadModule.Getter isUploadProcessesActive;

  @UploadModule.Action startUpload;

  async created() {
    await this.fetchUploadServers();
    await this.fetch();
    this.fetchUploadProcesses();
    this.timeoutHandler = setTimeout(this.timeoutFunction, this.intervalPeriod);
  }

  destroyed() {
    this.active = false;
    clearTimeout(this.timeoutHandler);
  }

  async timeoutFunction() {
    if (this.isUploadProcessesActive) {
      await this.fetchUploadProcesses();
    }
    if (this.active) {
      this.timeoutHandler = setTimeout(this.timeoutFunction, this.intervalPeriod);
    }
  }

  async onClickDelete(name: string) {
    try {
      await Dialog.confirm({
        title: '确认删除数据',
        message: name
      });

      this.deleteStorage(name);
      this.activeName = '';
    } catch (e) {
      console.log(e);
    }
  }

  async onClickUpload(name: string) {
    console.log('User clicked upload', name);
    this.showUploadDialog = true;
    this.recordFileNameToUpload = name;
    this.selectedUploadServer = '';
  }

  async onConfirmUpload(action, done) {
    if (action === 'confirm') {
      console.log('User confirmed upload', name);
      if (this.selectedUploadServer == '') {
        Toast('选择一个服务器');
        done(false);
      } else {
        await this.startUpload({ name: this.recordFileNameToUpload, remote: this.selectedUploadServer });
      }
    }
    done();
  }

  dataSizeFormatShort = dataSizeFormatShort;

  activeName = '';

  showUploadDialog = false;

  recordFileNameToUpload = '';

  selectedUploadServer = '';

  intervalPeriod = 500;

  timeoutHandler!: NodeJS.Timeout;

  active = true;
}
</script>

<style lang="stylus">
.center
  text-align center

.right
  text-align right
</style>
