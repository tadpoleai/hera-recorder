import {
  TBufferedTransport,
  TJSONProtocol,
  createHttpClient,
  createHttpConnection,
} from 'thrift';

import {
  TronService, IStatusArgs, DeviceInitializer, DeviceInformation,
} from './gen-ts';

import config from './tronServerConfig';

export { IDeviceInitializerArgs } from './gen-ts';

const options = {
  transport: TBufferedTransport,
  protocol: TJSONProtocol,
  https: false,
  headers: {
    Host: config.hostName,
    'Content-Type': 'text/plain',
  },
};

const connection = createHttpConnection(
  config.hostName,
  config.port,
  options,
);

const thriftClient = createHttpClient(
  TronService.Client,
  connection,
);

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

export { daemonStatus };

const connectionFailedResult: ISimpleResult = {
  error: 20,
  reason: 'Connection to daemon timed out',
};

const pendingResult: ISimpleResult = {
  error: 30,
  reason: 'Other command is pending',
};

let timedOutCounter = config.timedOutMs;

export async function getStatus(): Promise<ISimpleResult> {
  if (daemonStatus.pending.command || daemonStatus.pending.sync) {
    return pendingResult;
  }
  daemonStatus.pending.sync = true;

  try {
    const result = await thriftClient.get_status();
    daemonStatus.connected = true;
    daemonStatus.status = result.status;
    timedOutCounter = config.syncPeriodMs + config.timedOutMs;
    return { error: result.error, reason: result.reason };
  } catch (e) {
    daemonStatus.connected = false;
    return connectionFailedResult;
  } finally {
    daemonStatus.pending.sync = false;
  }
}

export async function start(devices: Array<IDevice>, storageFolder: string): Promise<ISimpleResult> {
  if (daemonStatus.pending.command) {
    return pendingResult;
  }
  daemonStatus.pending.command = true;

  try {
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
    const result = await thriftClient.start(deviceInitializers, storageFolder);
    daemonStatus.connected = true;
    daemonStatus.status = result.status;
    timedOutCounter = config.syncPeriodMs + config.timedOutMs;
    return { error: result.error, reason: result.reason };
  } catch (e) {
    console.log(e);
    daemonStatus.connected = false;
    return connectionFailedResult;
  } finally {
    daemonStatus.pending.command = false;
  }
}

export async function stop(): Promise<ISimpleResult> {
  if (daemonStatus.pending.command) {
    return pendingResult;
  }
  daemonStatus.pending.command = true;

  try {
    const result = await thriftClient.stop();
    daemonStatus.connected = true;
    daemonStatus.status = result.status;
    timedOutCounter = config.syncPeriodMs + config.timedOutMs;
    return { error: result.error, reason: result.reason };
  } catch (e) {
    daemonStatus.connected = false;
    return connectionFailedResult;
  } finally {
    daemonStatus.pending.command = false;
  }
}

export async function record(to: boolean): Promise<ISimpleResult> {
  if (daemonStatus.pending.command) {
    return pendingResult;
  }
  daemonStatus.pending.command = true;

  try {
    const result = await thriftClient.record_or_pause(to);
    daemonStatus.connected = true;
    daemonStatus.status = result.status;
    timedOutCounter = config.syncPeriodMs + config.timedOutMs;
    return { error: result.error, reason: result.reason };
  } catch (e) {
    daemonStatus.connected = false;
    return connectionFailedResult;
  } finally {
    daemonStatus.pending.command = false;
  }
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

async function sync() {
  const status = await getStatus();
  setTimeout(sync, config.syncPeriodMs);
}
sync();

setInterval(() => {
  if (timedOutCounter > 0) {
    timedOutCounter -= 1000;
  } else {
    daemonStatus.connected = false;
  }
}, 1000);
