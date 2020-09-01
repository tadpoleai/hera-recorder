const defaultHost = window.location.hostname;
// const defaultHost = '192.168.1.52';
const env = process.env.NODE_ENV || 'production';
let defaultPort = 9091;
if (env == 'production') {
  defaultPort = 9090;
}
const defaultHttps = false;
const defaultPath = '';

export default {
  defaultHost,
  defaultPort,
  defaultHttps,
  defaultPath
};
