<template lang="pug">
  img.device-data-image(
    ref="img"
  )
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from 'vue-property-decorator';

@Component({})
export default class JpegImage extends Vue {
  @Prop({ required: true }) private jpeg!: Buffer;

  created() {
    this.onJpegChanged(this.jpeg, new Buffer(''));
  }

  @Watch('jpeg')
  onJpegChanged(newValue: Buffer, oldValue: Buffer) {
    const newImage = new Image();
    newImage.src = 'data:image/jpeg;base64,' + newValue.toString('base64');
    newImage.onload = () => {
      ((this.$refs['img'] as Element) as any).src = newImage.src;
    };
    newImage.onerror = (e: any) => {
      console.warn('Can not show image', e);
    };
  }
}
</script>
