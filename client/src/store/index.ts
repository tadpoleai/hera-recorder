// store/index.ts

import Vue from 'vue';
import Vuex, { StoreOptions } from 'vuex';
import { RootState } from './types';

import { store as Main } from './modules/main';
import { store as Meta } from './modules/meta';
import { store as AcquisitionControl } from './modules/acquisitionControl';
import { store as AcquisitionSetting } from './modules/acquisitionSetting';
import { store as DeviceData } from './modules/deviceData';
import { store as DeviceParameter } from './modules/deviceParameter';
import { store as DiskUsage } from './modules/diskUsage';
import { store as ProfileEdit } from './modules/profileEdit';
import { store as Storage } from './modules/storage';
import { store as Network } from './modules/network';
import { store as Upload } from './modules/upload';
import { store as Preference } from './modules/preference';
import { store as Log } from './modules/log';

Vue.use(Vuex);

const store: StoreOptions<RootState> = {
  modules: {
    Main,
    Meta,
    AcquisitionControl,
    AcquisitionSetting,
    DeviceData,
    DeviceParameter,
    DiskUsage,
    ProfileEdit,
    Storage,
    Network,
    Upload,
    Preference,
    Log
  },
  strict: true
};
export default new Vuex.Store<RootState>(store);
