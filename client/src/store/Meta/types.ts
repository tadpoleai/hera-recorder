import { Hera } from '@/api';

export interface ParameterRule {
  name: string;
  type: string;
  label: string;
  defaultValue: string;
  mutable: boolean;
  options?: Array<string>;
  requirement: string;
  range?: {
    min: string;
    max: string;
    step: string;
  };
  regex?: string;
  comment: string;
}
export interface DeviceDescription {
  label: string;
  comment: string;
  parameters: Array<ParameterRule>;
}

export interface MetaState {
  isFetched: boolean;
  deviceDescriptionMap: Record<string, DeviceDescription>;
  daemonVersion: string;
}
