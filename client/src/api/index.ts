import { TBufferedTransport, TBinaryProtocol, createHttpClient, createHttpConnection } from 'thrift';

import 'error-polyfill';

import * as Hera from './gen-ts';

import config from './config';

interface PureResult {
  error: number;
  reason: string;
}

const status = {
  active: false,
  local: {
    profiles: [
      new Hera.Profile({
        name: '测试',
        author: '未知用户',
        devices: [
          new Hera.Device({
            type: 'lidar/velodyne',
            name: 'top',
            essentialParameters: [],
            optionalParameters: [],
            forward: true
          })
        ]
      })
    ],
    profileToEdit: new Hera.Profile({
      name: 'Test',
      author: 'Unknown',
      devices: []
    }),
    profileAddOrEdit: false,
    currentProfileIndex: 0,
    executor: 'NoName',
    place: 'NoPlace',
    onlineSlam: false
  },
  remoteConnected: false,
  remoteStatus: new Hera.Status({
    started: false,
    recording: false,
    storageName: '',
    profileName: '',
    diskUsedSpaceKB: 0,
    diskTotalSpaceKB: 0,
    devices: [],
    profiles: [],
    meta: new Hera.Meta({ deviceTypeMetas: [] })
  }),
  deviceDatas: new Array<Hera.DeviceData>(),
  slamResultValid: false,
  slamResult: new Buffer('')
};

function newClient(callback: (err: any | void) => void): Hera.Service.Client {
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

function apiWrapper<T0, T1>(apiName: string) {
  // eslint-disable-next-line
  return (arg0: T0, arg1: T1) =>
    // eslint-disable-next-line
    new Promise<PureResult>(async resolve => {
      const client = newClient((err: any) => {
        status.remoteConnected = false;
        const reason = `连接错误：${err.toString()}`;
        console.log(reason);
        resolve({
          error: 20,
          reason
        });
      });

      console.log('Sending Request');
      let result: Hera.Result;
      let resultData: Hera.ResultData;
      switch (apiName) {
        case 'get':
          result = await client.get();
          status.local.profiles = result.status.profiles;
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'start':
          result = await client.start((arg0 as unknown) as number, (arg1 as unknown) as string);
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'stop':
          result = await client.stop();
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'record':
          result = await client.record((arg0 as unknown) as boolean);
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'adjustParameters':
          result = await client.adjustParameters(
            (arg0 as unknown) as number,
            (arg1 as unknown) as Array<Hera.Parameter>
          );
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'updateProfiles':
          result = await client.updateProfiles(status.local.profiles);
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'getData':
          resultData = await client.getData();
          if (resultData.error === 0) {
            status.deviceDatas = resultData.deviceDatas;
            status.slamResultValid = resultData.slamResultValid;
            status.slamResult = resultData.slamResult;
          }
          resolve({ error: resultData.error, reason: resultData.reason });
          break;
        default:
          break;
      }
      resolve({ error: 20, reason: '非法操作' });
    });
}

export function formatResult(result: PureResult) {
  if (result.error === 0) {
    return '成功';
  }
  return `错误码: ${result.error.toString()}, 信息: ${result.reason}`;
}

const Api = {
  get: apiWrapper<void, void>('get'),
  start: apiWrapper<number, string>('start'),
  stop: apiWrapper<void, void>('stop'),
  record: apiWrapper<boolean, void>('record'),
  adjustParameters: apiWrapper<number, Array<Hera.Parameter>>('adjustParameters'),
  updateProfiles: apiWrapper<void, void>('updateProfiles'),
  getData: apiWrapper<void, void>('getData'),
  config,
  formatResult
};

export { Hera, Api, status };
