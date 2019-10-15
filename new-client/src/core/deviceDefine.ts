import rules from '@/core/inputRules';

const deviceTypes = [
  'Dummy',
  'Imu',
  'Gps',
  'Lidar',
  'Camera',
];

const parameterTypes = {
  Dummy: [
    'DummyRate',
    'DummyValue',
  ],
  Imu: [
    'Kernel',
    'BaudRate',
    'SerialMsgType',
  ],
  Lidar: [
    'IpAddress',
    'DataPort',
  ],
};

const parameterRules = {
  DummyRate: [rules.required, rules.number, rules.interger, rules.ltzero],
  DummyValue: [rules.required, rules.number, rules.interger],
  Kernel: [rules.required, rules.isKernel],
  BaudRate: [rules.required, rules.number, rules.ltzero],
  SerialMsgType: [rules.required, rules.number, rules.serialMsgType],
  IpAddress: [rules.required, rules.ipAddress],
  DataPort: [rules.required, rules.number, rules.port],
};

const maxDeviceNameLength = 32;

export default {
  deviceTypes,
  parameterTypes,
  parameterRules,
  maxDeviceNameLength,
};
