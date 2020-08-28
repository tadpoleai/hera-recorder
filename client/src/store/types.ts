// store/types.ts

import { MainState } from './Main/types';
import { MetaState } from './Meta/types';
import { AcquisitionSettingState } from './AcquisitionSetting/types';
import { AcquisitionControlState } from './AcquisitionControl/types';
import { ProfileEditState } from './ProfileEdit/types';
import { DeviceDataState } from './DeviceData/types';
import { DeviceParameterState } from './DeviceParameter/types';

export interface RootState {
  main: MainState;
  meta: MetaState;
  acquisitionSetting: AcquisitionSettingState;
  acquisitionControl: AcquisitionControlState;
  deviceData: DeviceDataState;
  deviceParameter: DeviceParameterState;
  profileEdit: ProfileEditState;
}
