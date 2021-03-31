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

  van-cell-group(
    title="储存容量"
  )
    DiskUsage(
      :refresh="true"
    )

  van-cell-group()
    div(slot="title" style="display: flex; justify-content: space-between;")
      span 机内数据
      span
        template(
          v-if="isMultiSelectionMode"
        )
          van-icon.title-middle-icon(
            name="upgrade"
            size="24px"
            :color="selectedMultiStorageDataList.length ? '#1989fa' : 'grey'"
            @click="onClickUploadMulti()"
          )
          van-icon.title-middle-icon(
            name="delete-o"
            size="24px"
            :color="selectedMultiStorageDataList.length ? '#ee0a24' : 'grey'"
            @click="onClickDeleteMulti()"
          )
        van-icon(
          name="bars"
          @click="onClickMultiMode"
          size="24px"
          v-bind:class="{ 'active-icon': isMultiSelectionMode }"
        )

    template(
      v-if="isMultiSelectionMode"
    )
      van-cell(
        v-for="(storage, index) in fetchedData"
        center
        @click="onClickStorageData(storage.name)"
      )
        div(style="margin-bottom: 4px; display: flex; justify-content: space-between;")
          van-tag(plain type="primary" style="margin-top: 4px;")
            | {{storage.name}}
          van-tag(plain type="success" style="margin-top: 4px;")
            | {{dataSizeFormatShort(storage.size)}}
        van-icon.title-right-icon(
          slot="right-icon"
          size="large"
          :name="isStorageSelected(storage.name) ? 'checked' : 'circle'"
          :color="isStorageSelected(storage.name) ? '#1989fa' : 'grey'"
        )

    van-collapse(
      v-else
      v-model="selectedSingleStorageData"
      accordion
    )
      //- For storage record files
      van-collapse-item(
        v-for="(storage, index) in fetchedData"
        :title="storage.name"
      )
        div(slot="title" style="display: flex; justify-content: space-between;")
          van-tag(plain type="primary" style="margin-top: 4px;")
            | {{storage.name}}
          van-tag(plain type="success" style="margin-top: 4px;")
            | {{dataSizeFormatShort(storage.size)}}
        van-cell
          van-row()
            van-col(span="12")
              van-tag(type="primary" size="medium") 采集开始时间
            van-col.right(span="12")
              van-tag(type="primary" size="medium") 采集结束时间
          van-row()
            van-col(span="12")
              van-tag(plain) {{storage.startTime}}
            van-col.right(span="12")
              van-tag(plain) {{storage.endTime}}
          van-row()
            van-col(span="13")
              van-tag(type="primary" size="medium") 传感器名称
            van-col.right(span="5")
              van-tag(type="primary" size="medium") 消息数量
            van-col.right(span="6")
              van-tag(type="primary" size="medium") 占用容量
          van-row(v-for="d in storage.devices")
            van-col(span="13")
              van-tag(plain) {{d.typeName}}
            van-col.right(span="5")
              van-tag(plain) {{d.messageNum}}
            van-col.right(span="6")
              van-tag(plain) {{dataSizeFormatShort(d.dataSize)}}
        van-cell
          van-row(style="margin: 1em 0 0 0")
            van-col.center(span="12")
              van-button(
                type="danger"
                size="small"
                @click="onClickDeleteSingle(storage.name)"
              ) 删除
            van-col.center(span="12")
              van-button(
                type="primary"
                size="small"
                @click="onClickUploadSingle(storage.name)"
              ) 上传

</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { namespace } from 'vuex-class';
import { Toast, Dialog } from 'vant';
import { dataSizeFormatShort, dataSizeFormat } from '@/utils';

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

  @UploadModule.State uploadProcesses;
  @UploadModule.Action fetchUploadProcesses;
  @UploadModule.Getter isUploadProcessesActive;

  @UploadModule.Mutation setFilesToUpload;

  async created() {
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

  onClickMultiMode() {
    if (this.isMultiSelectionMode) {
      this.isMultiSelectionMode = false;
      this.selectedSingleStorageData = '';
    } else {
      this.isMultiSelectionMode = true;
      this.selectedMultiStorageDataList = [];
    }
  }

  isStorageSelected(name: string) {
    return this.selectedMultiStorageDataList.includes(name);
  }

  onClickStorageData(name: string) {
    const index = this.selectedMultiStorageDataList.indexOf(name);
    if (index > -1) {
      this.selectedMultiStorageDataList.splice(index, 1);
    } else {
      this.selectedMultiStorageDataList.push(name);
    }
  }

  async onClickDeleteSingle(name: string) {
    try {
      await Dialog.confirm({
        title: '确认删除单个数据',
        message: name
      });

      this.deleteStorage([name]);
      this.selectedSingleStorageData = '';
    } catch (e) {
      console.log(e);
    }
  }

  async onClickDeleteMulti() {
    if (this.selectedMultiStorageDataList.length == 0) {
      return;
    }

    try {
      let message = '';
      const len = Math.min(8, this.selectedMultiStorageDataList.length);
      for (let i = 0; i < len; i++) {
        message += this.selectedMultiStorageDataList[i] + '\n';
      }
      if (this.selectedMultiStorageDataList.length > 8) {
        message += '... 等' + this.selectedMultiStorageDataList.length + '个数据';
      }
      await Dialog.confirm({
        title: '确认删除多个数据',
        message
      });

      this.deleteStorage(this.selectedMultiStorageDataList.slice());
      this.selectedMultiStorageDataList = [];
    } catch (e) {
      console.log(e);
    }
  }

  async onClickUploadSingle(name: string) {
    this.setFilesToUpload([name]);
    this.$router.push('upload');
  }

  async onClickUploadMulti() {
    this.setFilesToUpload(this.selectedMultiStorageDataList.slice());
    this.selectedMultiStorageDataList = [];
    this.$router.push('upload');
  }

  selectedSingleStorageData = '';

  selectedMultiStorageDataList = [] as Array<string>;

  isMultiSelectionMode = false;

  intervalPeriod = 500;

  timeoutHandler!: NodeJS.Timeout;

  active = true;

  // Methods
  dataSizeFormatShort = dataSizeFormatShort;

  dataSizeFormat = dataSizeFormat;
}
</script>

<style lang="stylus">
.center
  text-align center

.right
  text-align right

.title-right-icon
  margin-left 2px
  margin-right 0px

.title-middle-icon
  margin-left 2px
  margin-right 22px
</style>
