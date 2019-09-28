const TronErrno = {
  Success: 0,
  InStatusError: 10,
  ConnectionFailed: 20,

  InvalidDeviceType: 100,
  InvalidDeviceName: 101,
  InvalidDeviceId: 102,
  InvalidControlCommand: 103,
  EmptyDeviceList: 104,
  DevicesAlreadyCreated: 105,

  CanNotOpenEthernetDevice: 203,
  CanNotOpenUsbDevice: 204,
  CanNotOpenTtyDevice: 205,
  CanNotOpenSmbusDevice: 206,

  InvalidParameterType: 300,
  InsufficientParameters: 301,
  UnimplementedParameter: 302,
  InvalidParameterValue: 303,

  CanNotConnectSynchronizer: 400,
  GpsTimeShiftOutOfRange: 401,
  CameraTimeShiftOutOfRange: 402,
  LidarTimeShiftOutOfRange: 403,

  StorageFolderNoSet: 500,
  StorageFolderAlreadySet: 501,
  CanNotCreateFolder: 502,
  CanNotOpenFile: 503,
  WriteError: 504,

  DeviceNotReady: 600,
  DeviceAlreadyConnected: 601,
  DeviceAlreadyClosed: 602,

  NoNewData: 700,

  Reserved: 9999,
};

const TronErrnoString = {};
Object.keys(TronErrno).forEach((key) => {
  TronErrnoString[TronErrno[key]] = key;
});

export default {
  TronErrno,
  TronErrnoString,
};
