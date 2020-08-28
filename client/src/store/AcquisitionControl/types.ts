import { Hera } from '@/api';

export interface AcquisitionControlState {
  isFetched: boolean;
  fetchedData: Hera.AcquisitionStatus;
  isErrorCleared: boolean;
}
