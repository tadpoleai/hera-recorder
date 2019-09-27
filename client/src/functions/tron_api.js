import TronErrno from './tron_errno';

const tronApiUri = 'http://10.211.55.6:9090/tron';

const ConnectionFailedMsg = {
  error: TronErrno.ConnectionFailed,
  reason: 'Connection to Tron Daemon Failed',
};

async function createDevices(deviceInitializers) {
  try {
    const transport = new Thrift.TFramedTransport(tronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.create_devices(deviceInitializers);
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

async function getInformations() {
  try {
    const transport = new Thrift.TXHRTransport(tronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    const result = {
      error: 0,
      reason: 'OK',
      data: client.get_informations(),
    };
    return result;
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

async function setStorage(folder) {
  try {
    const transport = new Thrift.TXHRTransport(tronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.set_storage(folder);
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

async function adjustDeviceParameters(deviceId, parameters) {
  try {
    const transport = new Thrift.TXHRTransport(tronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.adjust_device_parameters(deviceId, parameters);
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

async function control(command) {
  try {
    const transport = new Thrift.TXHRTransport(tronApiUri);
    const protocol = new Thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.control(command, true, 0);
  } catch (error) {
    return ConnectionFailedMsg;
  }
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
