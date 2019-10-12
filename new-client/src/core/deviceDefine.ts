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
  ],
  Lidar: [
    'IpAddress',
    'Port',
  ],
};

const parameterRules = {
  DummyRate: [rules.required, rules.number, rules.interger, rules.ltzero],
  DummyValue: [rules.required, rules.number, rules.interger],
  Kernel: [rules.required, rules.isKernel],
  BaudRate: [rules.required, rules.number, rules.ltzero],
  IpAddress: [rules.required, rules.ipAddress],
  Port: [rules.required, rules.number, rules.port],
};

const maxDeviceNameLength = 32;

export default {
  deviceTypes,
  parameterTypes,
  parameterRules,
  maxDeviceNameLength,
};
