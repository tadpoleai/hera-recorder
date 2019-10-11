<template>
  <div>
    <p class="text-center headline">{{$t('noconnection')}}</p>
    <p class="text-center">
      <v-btn icon @click="reload" x-large dark>
        <v-icon>mdi-reload</v-icon>
      </v-btn>
    </p>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';

@Component
export default class ConnectionFailed extends Vue {
  reload(): void {
    window.location.reload(false);
  }
}
</script>
