import { TBufferedTransport, TBinaryProtocol, createHttpClient, createHttpConnection } from 'thrift';

import 'error-polyfill';

import * as Hera from './gen-ts';

import config from './config';

function connect(callback: (err: any | void) => void): Hera.Service.Client {
  const connection = createHttpConnection(config.defaultHost, config.defaultPort, {
    transport: TBufferedTransport,
    protocol: TBinaryProtocol,
    https: config.defaultHttps,
    path: config.defaultPath,
    headers: {
      Host: config.defaultHost
    }
  });
  connection.on('error', callback);
  return createHttpClient(Hera.Service.Client, connection);
}

export { Hera, connect };
