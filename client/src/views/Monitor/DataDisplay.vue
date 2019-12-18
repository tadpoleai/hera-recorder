<template>
  <div>
    <v-row>
      <v-col cols="6" class="pa-1">Seq</v-col>
      <v-col cols="6" class="pa-1">{{displayData.sequence}}</v-col>
    </v-row>
    <v-row>
      <v-col cols="12" class="pa-1">
        <v-card flat>
          <v-img v-if="displayData.valid && displayData.isJpeg" :src="imageSrc" />
        </v-card>
        <span v-if="displayData.valid && !displayData.isJpeg" v-html="stringInfo"></span>
      </v-col>
    </v-row>
  </div>
</template>

<script lang="ts">
import { Component, Vue, Prop } from "vue-property-decorator";
import { IDisplayData } from "@/core/daemonApi";

@Component
export default class DeviceListEdit extends Vue {
  @Prop()
  displayData!: IDisplayData;

  get imageSrc() {
    return "data:image/jpg;base64," + this.displayData.data.toString("base64");
  }

  get stringInfo() {
    return this.displayData.data.toString().replace(/\n/g, '<br/>');
  }
}
</script>
