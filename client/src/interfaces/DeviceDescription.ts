import ParameterRule from './ParameterRule';

interface DeviceDescription {
  label: string;
  comment: string;
  parameters: Array<ParameterRule>;
}

export default DeviceDescription;
