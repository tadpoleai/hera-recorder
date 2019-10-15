let hostName: string;
let port: number;
let useHttps: boolean;

if (process.env.NODE_ENV === 'production') {
  hostName = '10.0.0.1';
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
  syncPeriodMs: 6000,
  path: '/tron',
};
