import thrift from 'thrift';
import ControlCommand from './gen-js/tron_types';
import TronServiceClient from './gen-js/TronService';
import TronErrno from './tron_errno';

const ConnectionFailedMsg = {
  error: TronErrno.ConnectionFailed,
  reason: 'Connection to Tron Daemon Failed',
};

async function createDevices(deviceInitializers) {
  try {
    const transport = new thrift.TXHRTransport('/tron');
    const protocol = new thrift.TJSONProtocol(transport);
    const client = new TronServiceClient(protocol);
    return client.create_devices(deviceInitializers);
  } catch (error) {
    return ConnectionFailedMsg;
  }
}

export default {
  createDevices,
};
