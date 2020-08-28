import { Hera } from '@/api';

export interface DeviceParameterState {
  isFetched: boolean;
  fetchedData: Array<Hera.DeviceAndParameters>;
}
