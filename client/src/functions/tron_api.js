import TronErrno from './tron_errno';

const TronApiUri = 'http://10.211.55.6:9090/tron';

const ConnectionFailedMsg = {
  error: TronErrno.TronErrno.ConnectionFailed,
  reason: 'Connection to Tron Daemon Failed',
};

function formatError(ret) {
  let msg;
  if (ret.error === TronErrno.TronErrno.Success) {
    msg = 'OK';
  } else {
    msg = 'Error';
    msg += ' :';
    msg += TronErrno.TronErrnoString[ret.error];
    msg += ', ';
    msg += ret.reason;
  }
  return msg;
}

const transport = new Thrift.TXHRTransport(TronApiUri);
const protocol = new Thrift.TJSONProtocol(transport);
const client = new TronServiceClient(protocol);

function createDevices(deviceInitializers) {
  return new Promise((resolve) => {
    client.create_devices(deviceInitializers, (result) => {
      if (result.error === undefined) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

function getInformations() {
  return new Promise((resolve) => {
    client.get_information((result) => {
      if (result.error === undefined) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

function setStorage(folder) {
  return new Promise((resolve) => {
    client.set_storage(folder, (result) => {
      if (result.error === undefined) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

function adjustDeviceParameters(deviceId, parameters) {
  return new Promise((resolve) => {
    client.adjust_device_parameters(deviceId, parameters, (result) => {
      if (result.error === undefined) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

function control(command) {
  return new Promise((resolve) => {
    client.control(command, (result) => {
      if (result.error === undefined) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

export default {
  TronErrno,
  ControlCommand,
  formatError,
  createDevices,
  getInformations,
  setStorage,
  adjustDeviceParameters,
  control,
};
