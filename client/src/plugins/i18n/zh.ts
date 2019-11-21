export default {
  title: 'HERA-数据采集平台',
  wayz: 'Wayz',
  noconnection: '连接采集平台失败',
  noDataText: '数据为空',

  Views: {
    home: '主页',
    acquisition: '采集控制',
    monitor: '状态监控',
    storage: '存储管理',
    about: '关于',
  },

  Actions: {
    start: '开始',
    stop: '终止',
    record: '录制',
    pause: '暂停',
    addDevice: '添加传感器',
    deleteDevice: '删除传感器',
    updateProfile: '更新配置',
    addProfile: '增加配置',
    deleteProfile: '删除配置',
  },

  Profile: {
    title: '采集配置',
    profiles: '采集配置',
    profileName: '配置名',
    deviceList: '设备列表',
  },

  Storage: {
    path: '储存路径',
    Path: {
      label: '输入储存路径',
      hint: '采集的数据将被储存在设置的路径下',
    },
  },

  Acquisition: {
    control: '采集控制',
    volume: '容量',
    deviceName: '传感器名',
  },

  InputRules: {
    required: '不能为空',
    onlyIdentifier: '不能包含特殊字符，可接受英文字母，数字和下划线',
    ipAddress: '请输入有效的IP地址',
    interger: '请输入整数',
    number: '请输入数字',
    ltzero: '请输入大于0的数字',
    isKernel: '请输入有效的内核名',
    port: '请输入有效的端口号，在0-65535之间',
    serialMsgType: '请输入有效的串口子类型号，在0-7之间',
  },

  Devices: {
    Type: {
      label: '类型',
    },
    Name: {
      label: '名称',
      hint: '设备储存时候的文件夹名',
    },
    Parameter: {
      Name: { label: '参数名' },
      Value: { label: '参数值' },
    },
    deviceTypeAndName: '类型与名称',
    parameters: '参数',
  },

  Button: {
    ok: '好的',
  },

  Result: {
    success: '操作成功',
  },
};
