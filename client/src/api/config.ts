const defaultHost = window.location.hostname;
//const defaultHost = "192.168.1.49";
const defaultPort = 9090;
const defaultHttps = false;
const defaultPath = '';
const syncPeriodMs = 5000;
const storagePeriodMs = 1000;
const dataPeriodMs = 300;

export default {
  defaultHost,
  defaultPort,
  defaultHttps,
  defaultPath,
  syncPeriodMs,
  storagePeriodMs,
  dataPeriodMs
};
