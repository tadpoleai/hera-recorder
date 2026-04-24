import { TBufferedTransport, TBinaryProtocol, createHttpClient, createHttpConnection } from 'thrift';

import 'error-polyfill';

import * as Hera from './gen-ts';

import store from '@/store/modules/main';

function connect(callback: (err: any | void) => void): Hera.Service.Client {
  const connection = createHttpConnection(store.hostUrl, store.hostPort, {
    transport: TBufferedTransport,
    protocol: TBinaryProtocol,
    https: false,
    path: '/hera',
    headers: {
      Host: store.hostUrl
    }
  });
  connection.on('error', callback);

  return createHttpClient(Hera.Service.Client, connection);
}

export { Hera, connect };
