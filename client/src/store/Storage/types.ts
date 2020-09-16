import { Hera } from '@/api';

export interface StorageState {
  isFetched: boolean;
  fetchedData: Hera.StorageStatus;
}
