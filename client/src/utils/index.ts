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

export function dataSizeFormat(datasizeKB: number): string {
  if (datasizeKB < 1024) {
    return datasizeKB.toFixed(0) + 'KiB';
  }

  if (datasizeKB < 1048576) {
    return (datasizeKB / 1024).toFixed(2) + 'MiB';
  }

  return (datasizeKB / 1048576).toFixed(2) + 'GiB';
}
