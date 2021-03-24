export function durationFormat(durationSec: number): string {
  const totalSec = durationSec + 0.1;

  const s = Math.floor(totalSec % 60);
  let ret = s + '秒 ';

  if (totalSec >= 60) {
    const m = Math.floor(totalSec / 60) % 60;
    ret = m + '分 ' + ret;
  }

  if (totalSec >= 3600) {
    const h = Math.floor(totalSec / 3600) % 24;
    ret = h + '小时 ' + ret;
  }

  if (totalSec >= 86400) {
    const d = Math.floor(totalSec / 86400);
    ret = d + '天 ' + ret;
  }

  return ret;
}

export function timestampFormat(tsSec: number, tsNsec: number): string {
  return new Date(tsSec * 1000 + tsNsec / 1000000).toISOString();
}

export function dataSizeFormat(datasize: number): string {
  if (datasize < 1024) {
    return datasize + 'B';
  }

  if (datasize < 1048576) {
    return (datasize / 1024).toFixed(1) + 'KiB';
  }

  if (datasize < 1048576 * 1024) {
    return (datasize / 1048576).toFixed(2) + 'MiB';
  }

  if (datasize < 1048576 * 1048576) {
    return (datasize / (1048576 * 1024)).toFixed(2) + 'GiB';
  }

  return (datasize / (1048576 * 1048576)).toFixed(2) + 'TiB';
}

export function frequencyFormat(frequency: number): string {
  if (frequency <= 1) {
    return frequency.toFixed(2) + 'Hz';
  } else if (frequency <= 10) {
    return frequency.toFixed(1) + 'Hz';
  } else {
    return frequency.toFixed(0) + 'Hz';
  }
}

export function dataSizeFormatShort(datasize: number): string {
  if (datasize < 1024) {
    return datasize + 'B';
  }

  if (datasize < 1048576) {
    return (datasize / 1024).toFixed(0) + 'K';
  }

  if (datasize < 1048576 * 1024) {
    return (datasize / 1048576).toFixed(1) + 'M';
  }

  if (datasize < 1048576 * 1048576) {
    return (datasize / (1048576 * 1024)).toFixed(1) + 'G';
  }

  return (datasize / (1048576 * 1048576)).toFixed(1) + 'T';
}

export function getHealth(rawMessage: string) {
  switch (rawMessage) {
    case 'Created':
      return { type: 'primary', text: '启动' };
    case 'Running':
      return { type: 'success', text: '运行' };
    case 'NoData':
      return { type: 'warning', text: '无数据' };
    case 'Aged':
      return { type: 'warning', text: '停滞' };
    case 'Stopped':
      return { type: 'primary', text: '终止' };
    case 'Error':
      return { type: 'danger', text: '出错' };
    default:
      return { type: 'danger', text: '未知' };
  }
}
