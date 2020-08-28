import { Hera } from '@/api';

export interface DeviceDataState {
  isFetched: boolean;
  fetchedData: Hera.DataStatus;
  isShowDetailDevice: boolean;
  selectDetailDeviceIndex: number;
}
