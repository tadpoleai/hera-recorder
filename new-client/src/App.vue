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

    <v-app-bar :color="appBarColor" app>
      <v-toolbar-title class="white--text">{{$t('title')}}</v-toolbar-title>
      <v-spacer />
      <span v-show="showAppBarMsg" class="white--text">{{$t('noconnection')}}</span>
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
        </v-list>
      </v-menu>
    </v-app-bar>

    <v-content app>
      <v-container fluid>
        <v-layout justify-center>
          <keep-alive>
            <router-view />
          </keep-alive>
        </v-layout>
      </v-container>
    </v-content>

    <v-bottom-navigation v-if="showBottomNav" dark shift app>
      <v-btn v-for="route in routeList" :key="route.name" :to="route.path">
        <span>{{$t(route.name)}}</span>
        <v-icon>{{ route.icon }}</v-icon>
      </v-btn>
    </v-bottom-navigation>
  </v-app>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import router from '@/router/router';
import { daemonStatus } from '@/core/tronApi';

@Component()
export default class App extends Vue {
  name = 'app';

  routeList = router.routeList;

  daemonStatus = daemonStatus;

  get showDrawer(): boolean {
    return !this.$vuetify.breakpoint.mdAndDown;
  }

  get showBottomNav(): boolean {
    return this.$vuetify.breakpoint.mdAndDown;
  }

  get appBarColor(): boolean {
    return this.daemonStatus.connected ? 'primary' : 'error';
  }

  get showAppBarMsg(): boolean {
    return !this.daemonStatus.connected;
  }

  setLocale(l: string): void {
    this.$i18n.locale = l;
  }
}
</script>
