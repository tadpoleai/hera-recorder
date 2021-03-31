// store/types.ts

import { State as MainState } from './modules/main';
import { State as MetaState } from './modules/meta';
import { State as AcquisitionSettingState } from './modules/acquisitionSetting';
import { State as AcquisitionControlState } from './modules/acquisitionControl';
import { State as DeviceDataState } from './modules/deviceData';
import { State as DeviceParameterState } from './modules/deviceParameter';
import { State as ProfileEditState } from './modules/profileEdit';
import { State as StorageState } from './modules/storage';
import { State as UploadState } from './modules/upload';
import { State as PreferenceState } from './modules/preference';
import { State as LogState } from './modules/log';

export interface RootState {
  Main: MainState;
  Meta: MetaState;
  AcquisitionSetting: AcquisitionSettingState;
  AcquisitionControl: AcquisitionControlState;
  DeviceData: DeviceDataState;
  DeviceParameter: DeviceParameterState;
  ProfileEdit: ProfileEditState;
  Storage: StorageState;
  Upload: UploadState;
  Preference: PreferenceState;
  Log: LogState;
}
