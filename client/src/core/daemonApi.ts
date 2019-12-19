import {
  TBufferedTransport,
  TJSONProtocol,
  createHttpClient,
  createHttpConnection,
} from 'thrift';

import {
  AcquisitionManager, IStatusArgs, DeviceInitializer, DeviceInformation, Result, ProfileResult,
} from './gen-ts';

import config from './serverConfig';

export interface IDisplayData {
  valid: boolean;
  isJpeg: number;
  sequence: number;
  timeSecond: number;
  timeNanosecond: number;
  data: Buffer;
}

export interface IParameter {
  type: string,
  value: string,
}

export interface IDevice {
  type: string,
  name: string,
  parameters: Array<IParameter>
}

export interface IProfile {
  name: string
  devices: Array<IDevice>
}

export interface IDaemonStatus {
  connected: boolean;
  status: IStatusArgs;
  profiles: Array<IProfile>;
}

export interface ISimpleResult {
  error: number,
  reason: string,
}

export interface IDeviceInfo {
  id: number,
  type: string,
  name: string,
  status: number,
  forward: boolean,
  volume: number,
  parameters: Array<IParameter>,
  data: IDisplayData,
  frequency: number;
  error: number,
  reason: string,
}

const daemonStatus: IDaemonStatus = {
  connected: false,
  status: {
    inited: false,
    record: false,
    folder: '',
    devices: [],
  },
  profiles: [],
};

const connectionFailedResult: ISimpleResult = {
  error: 20,
  reason: 'Connection to daemon timed out',
};

const pendingResult: ISimpleResult = {
  error: 30,
  reason: 'Other command is pending',
};

const connectionOptions = {
  transport: TBufferedTransport,
  protocol: TJSONProtocol,
  https: config.useHttps,
  path: config.path,
  headers: {
    Host: config.hostName,
    'Content-Type': 'text/plain',
  },
};

function newClient(callback: (e: any) => void): AcquisitionManager.Client {
  const connection = createHttpConnection(
    config.hostName,
    config.port,
    connectionOptions,
  );
  connection.on('error', callback);
  return createHttpClient(
    AcquisitionManager.Client,
    connection,
  );
}

function apiWrapper<T0, T1>(apiName: string) {
  return (arg0: T0, arg1: T1) => new Promise<ISimpleResult>((async (resolve) => {
    const client = newClient((err: any) => {
      if (err.captureStackTrace !== undefined) {
        daemonStatus.connected = false;
        resolve(connectionFailedResult);
      }
    });

    let result: Result | ISimpleResult;
    console.log('Sending Request');
    switch (apiName) {
      case 'get':
        result = await client.get(arg0 as unknown as boolean);
        daemonStatus.status = (result as Result).status;
        break;
      case 'start':
        {
          const devices = arg0 as unknown as Array<IDevice>;
          const deviceInitializers: Array<DeviceInitializer> = [];
          devices.forEach((device: IDevice) => {
            const deviceInitializer = new DeviceInitializer({
              name: device.name,
              type: device.type,
              parameters: new Map<string, string>(),
            });
            device.parameters.forEach((parameter: IParameter) => {
              deviceInitializer.parameters.set(parameter.type, parameter.value);
            });
            deviceInitializers.push(deviceInitializer);
          });
          result = await client.start(deviceInitializers, arg1 as unknown as string);
          daemonStatus.status = (result as Result).status;
        }
        break;
      case 'stop':
        result = await client.stop();
        daemonStatus.status = (result as Result).status;
        break;
      case 'record':
        result = await client.record(arg0 as unknown as boolean);
        daemonStatus.status = (result as Result).status;
        break;
      case 'adjust':
        result = await client.adjust(
          arg0 as unknown as number,
          arg1 as unknown as Map<string, string>,
        );
        daemonStatus.status = (result as Result).status;
        break;
      case 'getProfile':
        result = await client.getProfile();
        const profileJsons = (result as ProfileResult).profiles;
        daemonStatus.profiles = JSON.parse(profileJsons);
        break;
      case 'updateProfile':
        result = await client.updateProfile(
          JSON.stringify(daemonStatus.profiles),
        );
        break;
      default:
        result = connectionFailedResult;
        break;
    }

    daemonStatus.connected = true;

    resolve({ error: result.error, reason: result.reason });
  }));
}

export function toDeviceInfo(deviceInfos: Array<DeviceInformation>): Array<IDeviceInfo> {
  const resultList: Array<IDeviceInfo> = [];
  deviceInfos.forEach((rawInfo: DeviceInformation) => {
    const parameters: Array<IParameter> = [];
    rawInfo.parameters.forEach((value: string, key: string) => {
      parameters.push({ type: key, value });
    });
    const info: IDeviceInfo = {
      parameters,
      id: rawInfo.id,
      type: rawInfo.type,
      name: rawInfo.name,
      status: rawInfo.status,
      forward: rawInfo.forward,
      volume: rawInfo.volume,
      data: rawInfo.data,
      frequency: rawInfo.frequency,
      error: rawInfo.error,
      reason: rawInfo.reason,
    };
    resultList.push(info);
  });
  return resultList;
}

export function formatResult(result: ISimpleResult) {
  return `Error: ${result.error.toString()}; Reason: ${result.reason}`;
}

const get = apiWrapper<boolean, void>('get');
const start = apiWrapper<Array<IDevice>, string>('start');
const stop = apiWrapper<void, void>('stop');
const record = apiWrapper<boolean, void>('record');
const getProfile = apiWrapper<void, void>('getProfile');
const updateProfile = apiWrapper<void, void>('updateProfile');

export {
  get, start, stop, record, getProfile, updateProfile, daemonStatus,
};

async function sync() {
  const status = await get(false);
  console.log(status);
  console.log(daemonStatus);
  setTimeout(sync, daemonStatus.status.inited ? config.activeSyncPeriodMs : config.syncPeriodMs);
}

async function init() {
  await getProfile();
  sync();
}

init();
