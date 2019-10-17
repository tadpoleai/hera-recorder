import {
  TBufferedTransport,
  TJSONProtocol,
  createHttpClient,
  createHttpConnection,
} from 'thrift';

import {
  TronService, IStatusArgs, DeviceInitializer, DeviceInformation, Result,
} from './gen-ts';

import config from './tronServerConfig';

export interface IDaemonStatus {
  connected: boolean;
  status: IStatusArgs;
  pending: {
    command: boolean;
    sync: boolean;
  }
}

export interface ISimpleResult {
  error: number,
  reason: string,
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

export interface IDeviceInfo {
  id: number,
  type: string,
  name: string,
  status: string,
  is_forward: boolean,
  volume: number,
  parameters: Array<IParameter>,
  error: number,
  reason: string,
}

const daemonStatus: IDaemonStatus = {
  connected: true,
  status: {
    can_start: false,
    can_stop: false,
    is_error: false,
    can_record: false,
    can_pause: false,
    is_record: false,
    storage_folder: '',
    devices: [],
  },
  pending: {
    command: false,
    sync: false,
  },
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

function newClient(callback: (e: Error) => void): TronService.Client {
  const connection = createHttpConnection(
    config.hostName,
    config.port,
    connectionOptions,
  );
  connection.on('error', callback);
  return createHttpClient(
    TronService.Client,
    connection,
  );
}

function apiWrapper<T0, T1>(apiName: string) {
  return (arg0: T0, arg1: T1) => new Promise<ISimpleResult>((async (resolve) => {
    // Return Pending if other command is pending
    if (apiName === 'getStatus') {
      if (daemonStatus.pending.command || daemonStatus.pending.sync) {
        resolve(pendingResult);
      }
      daemonStatus.pending.sync = true;
    } else {
      if (daemonStatus.pending.command) {
        resolve(pendingResult);
      }
      daemonStatus.pending.command = true;
    }

    const client = newClient((err: Error) => {
      console.log(`error${err}`);
      if (apiName === 'getStatus') {
        daemonStatus.pending.sync = false;
      } else {
        daemonStatus.pending.command = false;
      }
      daemonStatus.connected = false;
      resolve(connectionFailedResult);
    });

    let result: Result | ISimpleResult;
    switch (apiName) {
      case 'getStatus':
        result = await client.get_status();
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
      case 'recordOrPause':
        result = await client.record_or_pause(arg0 as unknown as boolean);
        daemonStatus.status = (result as Result).status;
        break;
      case 'adjustDeviceParameters':
        result = await client.adjust_device_parameters(
            arg0 as unknown as number,
            arg1 as unknown as Map<string, string>,
        );
        daemonStatus.status = (result as Result).status;
        break;
      default:
        result = connectionFailedResult;
        break;
    }

    if (apiName === 'getStatus') {
      daemonStatus.pending.sync = false;
    } else {
      daemonStatus.pending.command = false;
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
      is_forward: rawInfo.is_forward,
      volume: rawInfo.volume,
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

const getStatus = apiWrapper<void, void>('getStatus');
const start = apiWrapper<Array<IDevice>, string>('start');
const stop = apiWrapper<void, void>('stop');
const recordOrPause = apiWrapper<boolean, void>('recordOrPause');

export {
  getStatus, start, stop, recordOrPause, daemonStatus,
};

async function sync() {
  const status = await getStatus();
  console.log(status);
  setTimeout(sync, config.syncPeriodMs);
}
sync();
