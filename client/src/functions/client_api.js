import TronApi from './tron_api';

const ClientInfo = {
  isConnectionLost: true,
  canCreate: true,
  canStart: false,
  canStop: false,
  canRecord: false,
  canPause: false,
  canSetStorage: false,
  devices: [],
};

async function doCheck() {
  const ret = await TronApi.getInformations();
  if (ret.error === 0) {
    ClientInfo.isConnectionLost = false;
    if (ret.devices[0]) {
      ClientInfo.canCreate = false;
      ClientInfo.canStart = ret.devices[0].status === 'Uninited';
      ClientInfo.canStop = true;
      ClientInfo.canRecord = !ret.devices[0].is_record && ret.devices[0].status === 'Inited';
      ClientInfo.canPause = ret.devices[0].is_record;
      ClientInfo.canSetStorage = true;
      ClientInfo.devices = ret.devices;
    } else {
      ClientInfo.canCreate = true;
      ClientInfo.canStart = false;
      ClientInfo.canStop = false;
      ClientInfo.canRecord = false;
      ClientInfo.canPause = false;
      ClientInfo.canSetStorage = false;
      ClientInfo.devices = [];
    }
  } else {
    ClientInfo.isConnectionLost = true;
  }
}

async function periodCheck() {
  await doCheck();
  console.log(ClientInfo);
  setTimeout(periodCheck, 5000);
}

periodCheck();

export default ClientInfo;
