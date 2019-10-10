<template>
  <v-app>
    <v-navigation-drawer v-if="showDrawer" permanent app>
      <v-list-item>
        <v-list-item-avatar>
          <v-img src="@/assets/logo.svg"></v-img>
        </v-list-item-avatar>

        <v-list-item-title>{{$t('wayz')}}</v-list-item-title>
      </v-list-item>

      <v-list>
        <v-list-item v-for="route in routeList" :key="route.name" :to="route.path">
          <v-list-item-icon>
            <v-icon>{{ route.icon }}</v-icon>
          </v-list-item-icon>

          <v-list-item-content>
            <v-list-item-title>{{ $t(route.name) }}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-app-bar color="primary" app>
      <v-toolbar-title>{{$t('title')}}</v-toolbar-title>
      <v-spacer />
      <v-menu>
        <template v-slot:activator="{ on }">
          <v-btn icon v-on="on">
            <v-icon>mdi-translate</v-icon>
          </v-btn>
        </template>
        <v-list>
          <v-list-item @click="setLocale('en')">English</v-list-item>
          <v-list-item @click="setLocale('zh')">中文</v-list-item>
          <v-list-item @click="setLocale('jp')">日本語</v-list-item>
        </v-list>
      </v-menu>
    </v-app-bar>

    <v-content>
      <v-container fluid>
        <v-layout justify-center>
          <keep-alive>
            <router-view />
          </keep-alive>
        </v-layout>
      </v-container>
    </v-content>

    <v-overlay :value="showOverlay">
      <p class="text-center">{{$t('noconnection')}}</p>
      <p class="text-center">
        <v-btn icon @click="reload">
          <v-icon>mdi-reload</v-icon>
        </v-btn>
      </p>
    </v-overlay>

    <v-bottom-navigation v-if="showBottomNav" dark shift>
      <v-btn v-for="route in routeList" :key="route.name" :to="route.path">
        <span>{{$t(route.name)}}</span>
        <v-icon>{{ route.icon }}</v-icon>
      </v-btn>
    </v-bottom-navigation>
  </v-app>
</template>

<script lang="ts">
import { Component, Vue } from "vue-property-decorator";
import router from "@/router/router";
import vuetify from "@/plugins/vuetify";
import { daemonStatus } from "@/core/daemonStatus";

@Component
export default class App extends Vue {
  name = "app";

  routeList = router.routeList;

  daemonStatus = daemonStatus;

  get showDrawer(): boolean {
    return !this.$vuetify.breakpoint.mdAndDown;
  }

  get showBottomNav(): boolean {
    return this.$vuetify.breakpoint.mdAndDown;
  }

  get showOverlay(): boolean {
    return !this.daemonStatus.connected;
  }

  setLocale(l: string): void {
    this.$i18n.locale = l;
  }

  reload(): void {
    window.location.reload(false);
  }
}
</script>
