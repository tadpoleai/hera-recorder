let hostName: string;
let port: number;
let useHttps: boolean;

if (process.env.NODE_ENV === 'production') {
  hostName = '10.0.0.1';
  port = 80;
  useHttps = false;
} else {
  console.log(window.location.hostname);
  hostName = window.location.hostname;
  port = 9090;
  useHttps = false;
}
export default {
  hostName,
  port,
  useHttps,
  syncPeriodMs: 6000,
  activeSyncPeriodMs: 1000,
  path: '/hera',
};
