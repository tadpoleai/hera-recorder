let hostName: string;
let port: number;
let useHttps: boolean;

hostName = window.location.hostname;
port = 9090;
useHttps = false;

export default {
  hostName,
  port,
  useHttps,
  syncPeriodMs: 6000,
  activeSyncPeriodMs: 1000,
  path: '/hera',
};
