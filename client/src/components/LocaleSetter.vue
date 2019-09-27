<template>
  <md-menu md-direction="bottom-start" md-align-trigger>
    <md-button md-menu-trigger class="md-icon-button">
      <md-icon :md-src='require("@/assets/icons/translate.svg")' />
    </md-button>

    <md-menu-content>
      <md-menu-item @click="setLocale('zh')">中文</md-menu-item>
      <md-menu-item @click="setLocale('en')">English</md-menu-item>
    </md-menu-content>
  </md-menu>
</template>

<script>
export default {
  name: 'locale-setter',
  methods: {
    setLocale(lang) {
      this.$i18n.locale = lang;
    },
  },
};
</script>
