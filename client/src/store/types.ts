// store/types.ts

import { MainState } from './Main/types';
import { MetaState } from './Meta/types';
import { AcquisitionSettingState } from './AcquisitionSetting/types';
import { AcquisitionControlState } from './AcquisitionControl/types';
import { DeviceDataState } from './DeviceData/types';
import { DeviceParameterState } from './DeviceParameter/types';
import { ProfileEditState } from './ProfileEdit/types';
import { StorageState } from './Storage/types';
import { UploadState } from './Upload/types';

export interface RootState {
  main: MainState;
  meta: MetaState;
  acquisitionSetting: AcquisitionSettingState;
  acquisitionControl: AcquisitionControlState;
  deviceData: DeviceDataState;
  deviceParameter: DeviceParameterState;
  profileEdit: ProfileEditState;
  storage: StorageState;
  upload: UploadState;
}
