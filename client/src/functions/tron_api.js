import TronErrno from './tron_errno';

const TronApiUri = 'http://10.211.55.6:9090/tron';

const serverInfo = {
  connected: false,
  status: {
    devices: [],
  },
  pending: {
    syncLocal: false,
    command: false,
  },
};

function formatError(ret) {
  let message;
  if (ret.error === TronErrno.TronErrnoMap.Success) {
    message = 'OK';
  } else {
    message = 'Error';
    message += ': ';
    message += TronErrno.TronErrnoList[ret.error];
    if (ret.reason) {
      message += ', ';
      message += ret.reason;
    }
  }
  return message;
}

const ConnectionFailedMsg = {
  error: TronErrno.TronErrnoMap.ConnectionFailed,
  reason: 'Connection to daemon failed',
};

const RequestPendingMsg = {
  error: TronErrno.TronErrnoMap.RequestPending,
  reason: 'Other request is under processing',
};

function getStatus() {
  return new Promise((resolve) => {
    const transport = new Thrift.TXHRTransport(TronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    client.get_status((result) => {
      if (result.error === undefined) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

async function syncLocal() {
  if (serverInfo.pending.syncLocal === false && serverInfo.pending.command === false) {
    serverInfo.pending.syncLocal = true;
    const result = await getStatus();
    if (result.error === 0) {
      serverInfo.connected = true;
      serverInfo.status = result.status;
    } else {
      serverInfo.connected = false;
    }
    serverInfo.pending.syncLocal = false;
  }
}

function start(deviceInitializers, storageFolder) {
  return new Promise((resolve) => {
    if (serverInfo.pending.command === true) {
      resolve(RequestPendingMsg);
    } else {
      serverInfo.pending.command = true;
      const transport = new Thrift.TXHRTransport(TronApiUri);
      const protocol = new Thrift.TJSONProtocol(transport);
      const client = new TronServiceClient(protocol);
      client.start(deviceInitializers, storageFolder, (result) => {
        serverInfo.pending.command = false;
        if (result.error === undefined) {
          resolve(ConnectionFailedMsg);
        } else {
          serverInfo.status = result.status;
          resolve(result);
        }
      });
    }
  });
}

function stop() {
  return new Promise((resolve) => {
    if (serverInfo.pending.command === true) {
      resolve(RequestPendingMsg);
    } else {
      serverInfo.pending.command = true;
      const transport = new Thrift.TXHRTransport(TronApiUri);
      const protocol = new Thrift.TJSONProtocol(transport);
      const client = new TronServiceClient(protocol);
      client.stop((result) => {
        serverInfo.pending.command = false;
        if (result.error === undefined) {
          resolve(ConnectionFailedMsg);
        } else {
          serverInfo.status = result.status;
          resolve(result);
        }
      });
    }
  });
}

function recordOrPause(isRecord) {
  return new Promise((resolve) => {
    if (serverInfo.pending.command === true) {
      resolve(RequestPendingMsg);
    } else {
      serverInfo.pending.command = true;
      const transport = new Thrift.TXHRTransport(TronApiUri);
      const protocol = new Thrift.TJSONProtocol(transport);
      const client = new TronServiceClient(protocol);
      client.record_or_pause(isRecord, (result) => {
        serverInfo.pending.command = false;
        if (result.error === undefined) {
          resolve(ConnectionFailedMsg);
        } else {
          serverInfo.status = result.status;
          resolve(result);
        }
      });
    }
  });
}

async function periodSyncLocal() {
  await syncLocal();
  console.log('Period');
  console.log(serverInfo.status);
  setTimeout(periodSyncLocal, 15000);
}
periodSyncLocal();

export default {
  serverInfo,
  formatError,
  start,
  stop,
  recordOrPause,
};
