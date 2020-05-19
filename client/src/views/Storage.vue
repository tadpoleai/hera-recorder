<template lang="pug">
.monitor
  transition(
    name="van-fade"
  )
    van-cell-group(
      v-if="status.uploadInfo.uploadProcesses.length"
      title="数据上传"
    )
      transition(
        name="van-fade"
        v-for="p in status.uploadInfo.uploadProcesses"
      )
        upload(:process="p")

  van-cell-group
    div(slot="title" style="display: flex; justify-content: space-between;")
      span 磁盘信息
      van-icon(type="info" name="replay" size="large" @click="onClickUpdate()")

    van-cell
      div(style="display: flex; justify-content: space-between;")
        span 已用容量
        span {{diskVolume}}
      div
        van-progress(:percentage="diskVolumePercent"
        :show-pivot="false"
        stroke-width="8")

  van-cell-group
    div(slot="title" style="display: flex; justify-content: space-between;")
      span 本地数据
      van-icon(type="info" name="replay" size="large" @click="onClickUpdate()")

    van-collapse(
      v-model="activeName"
      accordion
    )
      van-collapse-item(
        v-for="(storage, index) in status.storageInfo.storageRecordFiles"
        :title="storage.name"
      )
        div(slot="title" style="display: flex; justify-content: space-between;")
          span {{storage.name}}
          span {{dataSizeFormat(storage.sizeKB)}}

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
            van-col.right(span="7") {{dataSizeFormat(d.dataSizeKB)}}
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

  van-dialog(
    v-model="showUploadDialog"
    title="上传数据"
    show-cancel-button
    :show-confirm-button="status.uploadInfo.remoteServers.length > 0"
    :beforeClose="beforeUploadDialogClose"
  )
    van-cell-group(title="数据名称")
      van-cell(:title="uploadStorageName")
    van-radio-group(v-model="uploadRemoteName")
      van-cell-group(title="服务器")
        template(v-if="status.uploadInfo.remoteServers.length > 0")
          van-cell(
            v-for="serverName in status.uploadInfo.remoteServers"
            :title="serverName"
            clickable
            @click="uploadRemoteName = serverName"
          )
            van-radio(
              slot="right-icon"
              :name="serverName"
            )
        template(v-else)
          van-cell() 没有可用服务器，请联系管理员


</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Hera, Api, status } from '@/api';
import { Toast, Dialog } from 'vant';
import { dataSizeFormat } from '@/utils';
import Upload from '@/views/Storage/Upload.vue';

@Component({
  components: { Upload }
})
export default class Storage extends Vue {
  constructor() {
    super();
    this.onClickUpdate();
    this.updateUploadInfo();
    this.intervalHandler = setInterval(this.updateUploadInfo, Api.config.storagePeriodMs);
  }

  destroyed() {
    clearInterval(this.intervalHandler);
  }

  updateUploadInfo() {
    Api.getUploadInfo();
  }

  async onClickUpdate() {
    Toast.loading({
      duration: 0,
      forbidClick: true,
      message: '正在加载本地数据列表'
    });

    Api.getStorage().then(result => {
      if (result.error !== 0) {
        Toast.fail({
          message: Api.formatResult(result),
          duration: 0,
          closeOnClick: true
        });
      } else {
        Toast.success('加载成功');
      }
    });
  }

  async onClickDelete(name: string) {
    try {
      await Dialog.confirm({
        title: '删除数据',
        message: name
      });

      Api.deleteStorage(name).then(result => {
        if (result.error !== 0) {
          Toast.fail({
            message: Api.formatResult(result),
            duration: 0,
            closeOnClick: true
          });
        } else {
          Toast.success('删除成功');
        }
      });

      this.activeName = '';
    } catch (e) {
      e;
    }
  }

  onClickUpload(name: string) {
    this.showUploadDialog = true;
    this.uploadStorageName = name;
  }

  async beforeUploadDialogClose(action: string, done: any) {
    if (action == 'confirm') {
      if (!this.uploadRemoteName) {
        Toast.fail({
          message: '请选择服务器'
        });
        done(false);
        return;
      }

      const req = new Hera.UploadRequest({
        name: this.uploadStorageName,
        remote: this.uploadRemoteName,
        compress: false
      });

      Api.startUpload(req);
      this.activeName = '';
      done();
    }
    done();
  }

  get diskVolumePercent() {
    if (!this.status.storageInfo.diskTotalSpaceKB) {
      return 100;
    } else {
      return (this.status.storageInfo.diskUsedSpaceKB / this.status.storageInfo.diskTotalSpaceKB) * 100;
    }
  }

  get diskVolume() {
    return (
      (this.status.storageInfo.diskUsedSpaceKB / 1024 / 1024).toFixed(3) +
      'GiB / ' +
      (this.status.storageInfo.diskTotalSpaceKB / 1024 / 1024).toFixed(3) +
      'GiB'
    );
  }

  status = status;

  activeName = '';

  intervalHandler!: NodeJS.Timeout;

  dataSizeFormat = dataSizeFormat;

  showUploadDialog = false;

  uploadStorageName = '';

  uploadRemoteName = '';
}
</script>

<style lang="stylus">
.center
  text-align center

.right
  text-align right
</style>
