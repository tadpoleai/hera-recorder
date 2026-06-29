#!/usr/bin/env python3
"""
FC Custom Container HTTP Handler for hera-stitch.

从 OSS 下载原始 .hera 文件，判断采集模式后调用对应拼接工具，
将拼接结果（含 InstaJpegFramePacket）写回 OSS。
支持 Stream 模式（hera-storage-ingest-stream-hera）和
RecordDownload 模式（hera-storage-ingest-insta-video）。

Request JSON:
{
  "oss_endpoint":    "oss-cn-hangzhou-internal.aliyuncs.com",
  "oss_bucket":      "my-data-bucket",
  "src_oss_prefix":  "hera-data/session-20240101/",   // trailing slash required
  "dst_oss_prefix":  "hera-stitched/session-20240101/",
  "mode":            "stream" | "record_download" | "auto",  // default: auto
  "fps":             1,        // output frames per second, default: 1
  "quality":         3,        // JPEG quality 1-31 (lower = better), default: 3
  "src_fps":         30        // source H.264 fps (stream only), default: 30
}

Response JSON:
{
  "status":         "success" | "error",
  "dst_oss_prefix": "hera-stitched/session-20240101/",
  "mode":           "stream" | "record_download",
  "message":        "..."
}
"""

import glob
import http.server
import json
import logging
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time

import oss2

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    stream=sys.stdout,
)
log = logging.getLogger(__name__)

PORT = int(os.environ.get("FC_SERVER_PORT", 9001))

INSTA_VENDOR   = 0x0421
INSTA_VIDEO    = 0x0421   # InstaVideoPacket  (Stream mode H.264)
INSTA_JPEG     = 0x0423   # InstaJpegFramePacket (already stitched)
HERA_HEADER_SZ = 4 * 1024 * 1024


def _make_auth() -> oss2.Auth:
    key_id     = os.environ.get("OSS_ACCESS_KEY_ID")
    key_secret = os.environ.get("OSS_ACCESS_KEY_SECRET")
    token      = os.environ.get("OSS_SECURITY_TOKEN")
    if token:
        return oss2.StsAuth(key_id, key_secret, token)
    if key_id and key_secret:
        return oss2.Auth(key_id, key_secret)
    raise RuntimeError("OSS credentials not found.")


def download_prefix(bucket: oss2.Bucket, prefix: str, local_dir: str) -> list:
    """Download all objects under prefix; return list of local paths."""
    os.makedirs(local_dir, exist_ok=True)
    paths = []
    for obj in oss2.ObjectIterator(bucket, prefix=prefix):
        if obj.key.endswith("/"):
            continue
        rel = obj.key[len(prefix):]
        local = os.path.join(local_dir, rel)
        os.makedirs(os.path.dirname(local), exist_ok=True)
        bucket.get_object_to_file(obj.key, local)
        log.info("  downloaded: %s", obj.key)
        paths.append(local)
    return paths


def upload_prefix(bucket: oss2.Bucket, local_dir: str, oss_prefix: str):
    """Upload all files in local_dir to oss_prefix."""
    for root, _, files in os.walk(local_dir):
        for fname in files:
            local = os.path.join(root, fname)
            rel = os.path.relpath(local, local_dir)
            oss_key = oss_prefix.rstrip("/") + "/" + rel
            size = os.path.getsize(local)
            if size > 100 * 1024 * 1024:
                oss2.resumable_upload(bucket, oss_key, local,
                                      multipart_threshold=100 * 1024 * 1024,
                                      num_threads=4)
            else:
                bucket.put_object_from_file(oss_key, local)
            log.info("  uploaded: %s -> oss://%s", local, oss_key)


def _count_packets(hera_path: str) -> tuple:
    """Return (video_pkts, jpeg_pkts) counts from a .hera file."""
    video = jpeg = 0
    try:
        with open(hera_path, "rb") as f:
            f.seek(HERA_HEADER_SZ)
            while True:
                hdr = f.read(24)
                if len(hdr) < 24:
                    break
                length = struct.unpack_from("<I", hdr, 0)[0]
                vendor = struct.unpack_from("<H", hdr, 8)[0]
                msg    = struct.unpack_from("<H", hdr, 10)[0]
                f.read(max(0, length - 24))
                if vendor == INSTA_VENDOR:
                    if msg == INSTA_VIDEO:
                        video += 1
                    elif msg == INSTA_JPEG:
                        jpeg += 1
    except OSError:
        pass
    return video, jpeg


def detect_mode(src_dir: str) -> str:
    """
    Auto-detect capture mode from .hera files in src_dir.
    Returns 'stream', 'record_download', or raises RuntimeError.
    """
    hera_files = glob.glob(os.path.join(src_dir, "*.hera"))
    if not hera_files:
        raise RuntimeError("No .hera files found in source directory.")

    for hf in hera_files:
        video_pkts, _ = _count_packets(hf)
        if video_pkts > 0:
            log.info("Detected Stream mode (%d video packets in %s)", video_pkts,
                     os.path.basename(hf))
            return "stream"

    # No H.264 packets → RecordDownload (relies on .insv / .mp4 sidecar)
    mp4_files = glob.glob(os.path.join(src_dir, "*.insv")) + \
                glob.glob(os.path.join(src_dir, "*.mp4"))
    if mp4_files:
        log.info("Detected RecordDownload mode (%d mp4/insv files)", len(mp4_files))
        return "record_download"

    raise RuntimeError(
        "Cannot detect capture mode: no H.264 packets in .hera and no .mp4/.insv files."
    )


def run_stitch(src_dir: str, out_dir: str, mode: str,
               fps: float, quality: int, src_fps: float) -> tuple:
    """Run the appropriate stitching tool. Returns (success, message)."""
    os.makedirs(out_dir, exist_ok=True)
    hera_files = sorted(glob.glob(os.path.join(src_dir, "*.hera")))
    if not hera_files:
        return False, "No .hera files found."

    t0 = time.time()

    if mode == "stream":
        for hf in hera_files:
            out_hf = os.path.join(out_dir, os.path.basename(hf))
            cmd = [
                "hera-storage-ingest-stream-hera",
                "--input",  hf,
                "--output", out_hf,
                "--fps",    str(fps),
                "--quality", str(quality),
                "--src-fps", str(src_fps),
            ]
            log.info("Running: %s", " ".join(cmd))
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=7200)
            if r.returncode != 0:
                return False, f"ingest-stream-hera failed: {r.stderr.strip()}"

        # Copy non-.hera sidecar files (e.g. insta_session.json)
        for f in os.listdir(src_dir):
            if not f.endswith(".hera"):
                shutil.copy2(os.path.join(src_dir, f), os.path.join(out_dir, f))

    elif mode == "record_download":
        mp4_files = glob.glob(os.path.join(src_dir, "*.insv")) + \
                    glob.glob(os.path.join(src_dir, "*.mp4"))
        if not mp4_files:
            return False, "RecordDownload mode: no .insv/.mp4 files found."

        for hf in hera_files:
            out_hf = os.path.join(out_dir, os.path.basename(hf))
            cmd = [
                "hera-storage-ingest-insta-video",
                "--input",   hf,
                "--output",  out_hf,
                "--fps",     str(fps),
                "--quality", str(quality),
            ]
            log.info("Running: %s", " ".join(cmd))
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=7200)
            if r.returncode != 0:
                return False, f"ingest-insta-video failed: {r.stderr.strip()}"

        for f in os.listdir(src_dir):
            if not f.endswith(".hera"):
                shutil.copy2(os.path.join(src_dir, f), os.path.join(out_dir, f))
    else:
        return False, f"Unknown mode: {mode}"

    elapsed = time.time() - t0
    return True, f"stitching completed in {elapsed:.1f}s ({mode} mode)"


def handle_stitch(event: dict) -> dict:
    oss_endpoint   = event["oss_endpoint"]
    oss_bucket     = event["oss_bucket"]
    src_oss_prefix = event["src_oss_prefix"].rstrip("/") + "/"
    dst_oss_prefix = event["dst_oss_prefix"].rstrip("/") + "/"
    mode           = event.get("mode", "auto")
    fps            = float(event.get("fps", 1))
    quality        = int(event.get("quality", 3))
    src_fps        = float(event.get("src_fps", 30))

    auth   = _make_auth()
    bucket = oss2.Bucket(auth, oss_endpoint, oss_bucket)

    with tempfile.TemporaryDirectory(prefix="hera_stitch_") as workdir:
        src_local = os.path.join(workdir, "src")
        out_local = os.path.join(workdir, "out")

        log.info("Step 1: downloading from oss://%s%s", oss_bucket, src_oss_prefix)
        download_prefix(bucket, src_oss_prefix, src_local)

        if mode == "auto":
            mode = detect_mode(src_local)

        log.info("Step 2: stitching (mode=%s, fps=%.1f)", mode, fps)
        ok, msg = run_stitch(src_local, out_local, mode, fps, quality, src_fps)
        if not ok:
            return {"status": "error", "dst_oss_prefix": dst_oss_prefix,
                    "mode": mode, "message": msg}

        log.info("Step 3: uploading to oss://%s%s", oss_bucket, dst_oss_prefix)
        upload_prefix(bucket, out_local, dst_oss_prefix)

    return {"status": "success", "dst_oss_prefix": dst_oss_prefix,
            "mode": mode, "message": msg}


class RequestHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        log.info(fmt, *args)

    def _send_json(self, code: int, body: dict):
        data = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length)
        try:
            event = json.loads(raw) if raw else {}
        except json.JSONDecodeError as e:
            self._send_json(400, {"status": "error", "message": f"Invalid JSON: {e}"})
            return

        required = {"oss_endpoint", "oss_bucket", "src_oss_prefix", "dst_oss_prefix"}
        missing = required - event.keys()
        if missing:
            self._send_json(400, {"status": "error",
                                  "message": f"Missing fields: {missing}"})
            return

        try:
            result = handle_stitch(event)
        except Exception as e:
            log.exception("Stitching failed")
            result = {"status": "error", "message": str(e)}

        self._send_json(200 if result["status"] == "success" else 500, result)

    def do_GET(self):
        if self.path in ("/", "/health"):
            self._send_json(200, {"status": "ok"})
        else:
            self._send_json(404, {"status": "not found"})


def main():
    server = http.server.ThreadingHTTPServer(("0.0.0.0", PORT), RequestHandler)
    log.info("hera-stitch handler listening on port %d", PORT)
    server.serve_forever()


if __name__ == "__main__":
    main()
