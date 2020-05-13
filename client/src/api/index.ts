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
    profileIndex: 0,
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
      name: '',
      author: '',
      devices: []
    }),
    profileAddOrEdit: false,
    operatorInfo: new Hera.OperatorInfo({
      storagePath: '',
      operatorName: '',
      place: '',
      slam: false
    })
  },
  remoteConnected: false,
  remoteStatus: new Hera.Status({
    started: false,
    recording: false,
    operatorInfo: new Hera.OperatorInfo({
      storagePath: '',
      operatorName: '',
      place: '',
      slam: false
    }),
    diskUsedSpaceKB: 0,
    diskTotalSpaceKB: 0,
    devices: [],
    profiles: [],
    profileIndex: 0,
    meta: new Hera.Meta({ deviceTypeMetas: [] })
  }),
  storageInfo: new Hera.StorageInfo({
    storageRecordFiles: [],
    diskTotalSpaceKB: 0,
    diskUsedSpaceKB: 0
  }),
  uploadInfo: new Hera.UploadInfo({
    uploadProcesses: [],
    remoteServers: []
  }),
  deviceDatas: new Array<Hera.DeviceData>(),
  slamResultValid: false,
  slamResult: new Buffer(''),
  startTimeSec: 0,
  nowTimeSec: 0
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

      let result: Hera.Result;
      let resultData: Hera.ResultData;
      switch (apiName) {
        case 'get':
          result = await client.get();
          if (((arg0 as unknown) as boolean) == true) {
            status.local.operatorInfo = result.status.operatorInfo;
            status.local.profiles = result.status.profiles;
            status.local.profileIndex = result.status.profileIndex;
          }
          status.remoteStatus = result.status;
          status.remoteConnected = true;
          resolve({ error: result.error, reason: result.reason });
          break;
        case 'start':
          result = await client.start((arg0 as unknown) as Hera.OperatorInfo);
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
          result = await client.updateProfiles(status.local.profiles, status.local.profileIndex);
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
            status.startTimeSec = resultData.startTimeSec;
            status.nowTimeSec = resultData.nowTimeSec;
          }
          resolve({ error: resultData.error, reason: resultData.reason });
          break;
        case 'getStorage':
          status.storageInfo = await client.getStorage();
          console.log(status.storageInfo);
          resolve({ error: 0, reason: '' });
          break;
        case 'deleteStorage':
          status.storageInfo = await client.deleteStorage((arg0 as unknown) as string);
          console.log(status.storageInfo);
          resolve({ error: 0, reason: '' });
          break;
        case 'getUploadInfo':
          status.uploadInfo = await client.getUploadInfo();
          console.log(status.uploadInfo);
          resolve({ error: 0, reason: '' });
          break;
        case 'startUpload':
          status.uploadInfo = await client.operateUpload(
            Hera.UploadOperationType.Start,
            (arg0 as unknown) as Hera.UploadRequest
          );
          console.log(status.uploadInfo);
          resolve({ error: 0, reason: '' });
          break;
        case 'completeUpload':
          status.uploadInfo = await client.operateUpload(
            Hera.UploadOperationType.Complete,
            (arg0 as unknown) as Hera.UploadRequest
          );
          console.log(status.uploadInfo);
          resolve({ error: 0, reason: '' });
          break;
        case 'retryUpload':
          status.uploadInfo = await client.operateUpload(
            Hera.UploadOperationType.Retry,
            (arg0 as unknown) as Hera.UploadRequest
          );
          console.log(status.uploadInfo);
          resolve({ error: 0, reason: '' });
          break;
        case 'abortUpload':
          status.uploadInfo = await client.operateUpload(
            Hera.UploadOperationType.Abort,
            (arg0 as unknown) as Hera.UploadRequest
          );
          console.log(status.uploadInfo);
          resolve({ error: 0, reason: '' });
          break;
        default:
          console.log('Invalid Operation');
          break;
      }
      resolve({ error: 20, reason: '非法操作' });
    });
}

export function formatResult(result: PureResult) {
  if (result.error === 0) {
    return '成功';
  }
  return `错误码: ${result.error.toString()} \n${result.reason}`;
}

const Api = {
  get: apiWrapper<boolean, void>('get'),
  start: apiWrapper<Hera.OperatorInfo, void>('start'),
  stop: apiWrapper<void, void>('stop'),
  record: apiWrapper<boolean, void>('record'),
  adjustParameters: apiWrapper<number, Array<Hera.Parameter>>('adjustParameters'),
  updateProfiles: apiWrapper<void, void>('updateProfiles'),
  getData: apiWrapper<void, void>('getData'),
  getStorage: apiWrapper<void, void>('getStorage'),
  deleteStorage: apiWrapper<string, void>('deleteStorage'),
  getUploadInfo: apiWrapper<void, void>('getUploadInfo'),
  startUpload: apiWrapper<Hera.UploadRequest, void>('startUpload'),
  completeUpload: apiWrapper<Hera.UploadRequest, void>('completeUpload'),
  retryUpload: apiWrapper<Hera.UploadRequest, void>('retryUpload'),
  abortUpload: apiWrapper<Hera.UploadRequest, void>('abortUpload'),
  config,
  formatResult
};

export { Hera, Api, status };
