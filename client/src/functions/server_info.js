import TronApi from './tron_api';

const ServerInfo = {
  isConnectionLost: true,
  information: {
    devices: [],
  },
};

async function doCheck() {
  const ret = await TronApi.getInformations();
  if (ret.error === 0) {
    ServerInfo.isConnectionLost = false;
    ServerInfo.information = ret;
  } else {
    ServerInfo.isConnectionLost = true;
  }
}

async function periodCheck() {
  await doCheck();
  console.log('Check');
  setTimeout(periodCheck, 3000);
}

periodCheck();

export default ServerInfo;
