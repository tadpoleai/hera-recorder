# camera/insta plugin (Phase 1)

Implemented files:
- plugin_entry.cpp
- plugin_data.hpp
- plugin_param.src
- plugin.cmake

Phase 1 behavior:
- Capture Insta preview bitstream callbacks as raw `InstaVideoPacket (0x0421)`.
- Capture Insta gyro callbacks as raw `InstaGyroPacket (0x0422)`.
- Keep convert minimal (`broken_data`) to prioritize stable recording.

Work modes:
- `Stream`: realtime preview/gyro callback capture to `.hera`.
- `RecordDownload`: SDK-only control path (start/stop recording), then download recording files on disconnect.

Build notes:
- Driver plugin requires Insta SDK headers and library (`camera/camera.h` + SDK `.so`).
- Specify SDK root by CMake option `-DINSTA_SDK_ROOT=/path` or env `INSTA_SDK_ROOT=/path`.
- If SDK is missing, only base plugin will be buildable.
