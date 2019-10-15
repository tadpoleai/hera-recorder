let hostName: string;
let port: number;
let useHttps: boolean;

if (process.env.NODE_ENV === 'production') {
  hostName = 'tron.newayz.com';
  port = 80;
  useHttps = false;
} else {
  hostName = '10.211.55.6';
  port = 9090;
  useHttps = false;
}
export default {
  hostName,
  port,
  useHttps,
  syncPeriodMs: 12000,
  timedOutMs: 5000,
  path: '/tron',
};
