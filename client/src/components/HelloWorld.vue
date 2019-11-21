<template>
  <v-container>
    <v-layout text-center wrap>
      <v-flex xs12 mb-5>
        <h2 class="headline font-weight-bold mb-3">{{sampleText}}</h2>

        <v-layout justify-center>
          <a
            v-for="(entry, i) in entries"
            :key="i"
            :href="entry.href"
            class="subheading mx-3"
            target="_blank"
          >
            {{ entry.text }}
          </a>
        </v-layout>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';

interface IHrefEntry {
    text:string,
    href:string,
}

@Component
export default class HelloWorld extends Vue {
  sampleText:string = 'Circle Lin Eat Meat';

  entries:IHrefEntry[] = [
    {
      text: 'KiraFan Wiki',
      href: 'https://wiki.kirafan.moe/',
    },
    {
      text: 'KiraFan Dream Server',
      href: 'https://dream.kirafan.moe/',
    },
  ]
}
</script>
