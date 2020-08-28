// store/index.ts

import Vue from 'vue';
import Vuex, { StoreOptions } from 'vuex';
import { RootState } from './types';

import { Main } from './Main';
import { Meta } from './Meta';
import { AcquisitionControl } from './AcquisitionControl';
import { AcquisitionSetting } from './AcquisitionSetting';
import { DeviceData } from './DeviceData';
import { DeviceParameter } from './DeviceParameter';
import { DiskUsage } from './DiskUsage';
import { ProfileEdit } from './ProfileEdit';

Vue.use(Vuex);

const store: StoreOptions<RootState> = {
  modules: {
    Main,
    Meta,
    AcquisitionControl,
    AcquisitionSetting,
    DiskUsage,
    DeviceData,
    DeviceParameter,
    ProfileEdit
  },
  strict: true
};
export default new Vuex.Store<RootState>(store);
