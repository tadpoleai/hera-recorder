import { Hera } from '@/api';

export interface ParameterRule {
  name: string;
  type: string;
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

export interface MetaState {
  isFetched: boolean;
  deviceRulesMap: Record<string, Array<ParameterRule>>;
}
