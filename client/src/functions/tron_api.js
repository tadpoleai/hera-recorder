import TronErrno from './tron_errno';

const TronApiUri = 'http://10.211.55.6:9090/tron';

const ConnectionFailedMsg = {
  error: TronErrno.ConnectionFailed,
  reason: 'Connection to Tron Daemon Failed',
};

const transport = new Thrift.TXHRTransport(TronApiUri);
const protocol = new Thrift.TJSONProtocol(transport);
const client = new TronServiceClient(protocol);

function createDevices(deviceInitializers) {
  return new Promise((resolve) => {
    client.create_devices(deviceInitializers, (result) => {
      if (result instanceof Error) {
        resolve(ConnectionFailedMsg);
      } else {
        resolve(result);
      }
    });
  });
}

function getInformations() {
  return new Promise((resolve) => {
    client.create_devices([], (result) => {
      console.log(result.error);
      console.log(typeof result);
      resolve({
        error: 0,
        reason: 'OK',
        devices: result,
      });
    });
  });
}

async function setStorage(folder) {
  try {
    const transport = new Thrift.TXHRTransport(TronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.set_storage(folder);
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

async function adjustDeviceParameters(deviceId, parameters) {
  try {
    const transport = new Thrift.TXHRTransport(TronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.adjust_device_parameters(deviceId, parameters);
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

async function control(command) {
  return new Promise((resolve) => {
    client.control(command, (result) => {
      if (result instanceof Error) {
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
  createDevices,
  getInformations,
  setStorage,
  adjustDeviceParameters,
  control,
};
