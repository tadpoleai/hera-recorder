import { Hera } from '@/api';

export interface DiskUsageState {
  isFetched: boolean;
  fetchedData: Hera.DiskUsageStatus;
}
