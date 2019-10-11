import rules from '@/core/inputRules';

const deviceTypes = [
  'dummy',
  'imu',
  'gps',
  'lidar',
  'camera',
];

const parameterTypes = {
  dummy: [
    'dummyRate',
    'dummyValue',
  ],
  imu: [
    'kernel',
    'baudRate',
  ],
  lidar: [
    'ipAddress',
    'port',
  ],
};

const parameterRules = {
  dummyRate: [rules.required, rules.number, rules.ltzero],
  dummyValue: [rules.required, rules.number],
  kernel: [rules.required, rules.isKernel],
  baudRate: [rules.required, rules.number, rules.ltzero],
  ipAddress: [rules.required, rules.ipAddress],
  port: [rules.required, rules.number, rules.port],
};

export default {
  deviceTypes,
  parameterTypes,
  parameterRules,
};
