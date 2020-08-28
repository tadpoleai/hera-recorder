import { Hera } from '@/api';

export interface AcquisitionSettingState {
  isFetched: boolean;
  fetchedData: Hera.AcquisitionSetting;
  isProfileEditing: boolean;
  profileEditingIndex: number;
}
