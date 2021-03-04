export interface MainState {
  isConnectionErrored: boolean;
  connectionErrorReason: string;

  hostUrl: string;
  hostPort: number;
}
