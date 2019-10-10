import {
  TBufferedTransport,
  TJSONProtocol,
  createHttpClient,
  createHttpConnection,
} from 'thrift';

import {
  TronService, IStatusArgs, IDeviceInitializerArgs, DeviceInitializer,
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

export interface DaemonStatus {
  connected: boolean;
  status: IStatusArgs;
  pending: {
    command: boolean;
    sync: boolean;
  }
}

export interface SimpleResult {
  error: number,
  reason: string,
}

const daemonStatus: DaemonStatus = {
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

const connectionFailedResult: SimpleResult = {
  error: 20,
  reason: 'Connection to daemon timed out',
};

const pendingResult: SimpleResult = {
  error: 30,
  reason: 'Other command is pending',
};

let timedOutCounter = config.timedOutMs;

export async function getStatus(): Promise<SimpleResult> {
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

export async function start(devices: Array<IDeviceInitializerArgs>, storageFolder: string): Promise<SimpleResult> {
  if (daemonStatus.pending.command) {
    return pendingResult;
  }
  daemonStatus.pending.command = true;

  try {
    const deviceInitializers: Array<DeviceInitializer> = [];
    devices.forEach((device: IDeviceInitializerArgs) => {
      deviceInitializers.push(new DeviceInitializer(device));
    });
    const result = await thriftClient.start(deviceInitializers, storageFolder);
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

export async function stop(): Promise<SimpleResult> {
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
  console.log(timedOutCounter);
}, 1000)