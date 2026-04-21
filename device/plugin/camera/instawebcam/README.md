# camera/instawebcam plugin (Phase 1)

Purpose:
- Linux/Jetson fallback for Insta capture when CameraSDK is unavailable on aarch64.
- Uses V4L2 (`/dev/video*`) to read webcam frames and stores raw packets to `.hera`.

Data:
- `InstaWebcamVideoPacket (0x0431)` for raw frame payload (MJPG/YUYV).

Notes:
- This plugin does not provide gyro data.
- Convert path stays minimal (`broken_data`) for Phase 1 stability-first recording.
