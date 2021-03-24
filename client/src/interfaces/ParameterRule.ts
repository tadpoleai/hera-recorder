interface ParameterRule {
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

export default ParameterRule;
