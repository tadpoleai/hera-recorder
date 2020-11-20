import { Hera } from '@/api';

export interface UploadState {
  isUploadServersFetched: boolean;
  uploadServers: Array<string>;

  uploadProcesses: Array<Hera.UploadProcess>;
}
