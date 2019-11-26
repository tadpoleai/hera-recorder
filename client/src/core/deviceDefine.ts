import rules from '@/core/inputRules';

const deviceTypes = [
  'dummy/foobar',
  'imu/aceinna',
  'gnss/serialsync',
  'lidar/velodyne',
  'camera/flir',
];

const parameterTypes = {
  'dummy/foobar': [
    'DummyRate',
    'DummyValue',
  ],
  'imu/aceinna': [
    'Kernel',
    'BaudRate',
    'SerialMsgType',
  ],
  'lidar/velodyne': [
    'IpAddress',
    'DataPort',
  ],
  'camera/flir': [
    'IpAddress',
  ],
  'gnss/serialsync': [
    'Kernel',
    'KernelAuxiliary',
    'BaudRate',
    'BaudRateAuxiliary',
    'SerialMsgType',
  ]
};

const parameterRules = {
  DummyRate: [rules.required, rules.number, rules.interger, rules.ltzero],
  DummyValue: [rules.required, rules.number, rules.interger],
  Kernel: [rules.required, rules.isKernel],
  KernelAuxiliary: [rules.required, rules.isKernel],
  BaudRate: [rules.required, rules.number, rules.ltzero],
  BaudRateAuxiliary: [rules.required, rules.number, rules.ltzero],
  SerialMsgType: [rules.required, rules.number, rules.serialMsgType],
  IpAddress: [rules.required, rules.ipAddress],
  DataPort: [rules.required, rules.number, rules.port],
};

const maxDeviceNameLength = 32;

const deviceIconTypes = {
  'dummy/foobar': 'mdi-ethereum',
  'imu/aceinna': 'mdi-compass',
  'gnss/serialsync': 'mdi-satellite-variant',
  'lidar/velodyne': 'mdi-hazard-lights',
  'camera/flir': 'mdi-camera',
};

export default {
  deviceTypes,
  parameterTypes,
  parameterRules,
  maxDeviceNameLength,
  deviceIconTypes
};
