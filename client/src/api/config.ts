const defaultHost = window.location.hostname;
// const defaultHost = '192.168.1.62';
const env = process.env.NODE_ENV || 'production';
let defaultPort = 9090;
if (env != 'production') {
  defaultPort = 9091;
}
const defaultHttps = false;
const defaultPath = '';

export default {
  defaultHost,
  defaultPort,
  defaultHttps,
  defaultPath
};
