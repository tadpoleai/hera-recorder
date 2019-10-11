import i18n from '@/plugins/i18n';

const reIpAddress = /^((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})(\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})){3}$/;

export default {
  required: (v: string) => (v.length > 0) || i18n.t('InputRules.required'),
  onlyIdentifier: (v: string) => (v.match(/^[0-9a-zA-Z_]*$/) !== null) || i18n.t('InputRules.onlyIdentifier'),
  ipAddress: (v: string) => (v.match(reIpAddress) !== null) || i18n.t('InputRules.ipAddress'),
  interger: (v: string) => (Number.isInteger(Number(v))) || i18n.t('InputRules.interger'),
  number: (v: string) => (!Number.isNaN(Number(v))) || i18n.t('InputRules.number'),
  ltzero: (v: string) => (Number(v) > 0) || i18n.t('InputRules.ltzero'),
  isKernel: (v: string) => (v.match(/^[0-9a-zA-Z_\-\/]*$/) !== null) || i18n.t('InputRules.isKernel'),
  port: (v: string) => (Number(v) < 65536 && Number(v) >= 0) || i18n.t('InputRules.port'),
};
