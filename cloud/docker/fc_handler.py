#!/usr/bin/env python3
"""
FC Custom Container HTTP Handler for hera-convert.

Alibaba Cloud FC custom container runtime — listens on port 9000 and handles
POST requests containing the OSS source/destination paths. For each request:
  1. Download the hera data folder from OSS to a local temp directory
  2. Run hera-convert to produce a ROS bag file
  3. Upload the resulting .bag file back to OSS
  4. Clean up and return the result

Request JSON schema:
{
  "oss_endpoint":     "oss-cn-hangzhou-internal.aliyuncs.com",
  "oss_bucket":       "my-data-bucket",
  "src_oss_prefix":   "hera-data/session-20240101/",   // trailing slash required
  "dst_oss_path":     "rosbag/session-20240101.bag",
  "remap_oss_path":   "configs/remap-tron4.json",      // optional
  "start_time":       0,                               // optional, seconds
  "duration":         0                                // optional, seconds
}

Response JSON schema:
{
  "status":       "success" | "error",
  "dst_oss_path": "rosbag/session-20240101.bag",
  "message":      "..."
}
"""

import http.server
import json
import logging
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time

import oss2

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    stream=sys.stdout,
)
log = logging.getLogger(__name__)

PORT = int(os.environ.get("FC_SERVER_PORT", 9000))

# OSS auth: FC runtime injects credentials via environment variables.
# When running on ECS/FC with a RAM role, oss2.StsAuth / oss2.CredentialProviderAuth
# picks them up automatically; otherwise fall back to key/secret from env.
def _make_auth() -> oss2.Auth:
    access_key_id     = os.environ.get("OSS_ACCESS_KEY_ID")
    access_key_secret = os.environ.get("OSS_ACCESS_KEY_SECRET")
    security_token    = os.environ.get("OSS_SECURITY_TOKEN")  # STS token (FC runtime)

    if security_token:
        return oss2.StsAuth(access_key_id, access_key_secret, security_token)
    if access_key_id and access_key_secret:
        return oss2.Auth(access_key_id, access_key_secret)
    raise RuntimeError(
        "OSS credentials not found. Set OSS_ACCESS_KEY_ID / OSS_ACCESS_KEY_SECRET "
        "env vars, or attach a RAM role to the FC service."
    )


def download_oss_prefix(bucket: oss2.Bucket, oss_prefix: str, local_dir: str):
    """Download all objects under oss_prefix to local_dir, preserving relative paths."""
    os.makedirs(local_dir, exist_ok=True)
    total = 0
    for obj in oss2.ObjectIterator(bucket, prefix=oss_prefix):
        if obj.key.endswith("/"):
            continue  # skip directory markers
        relative = obj.key[len(oss_prefix):]
        local_path = os.path.join(local_dir, relative)
        os.makedirs(os.path.dirname(local_path), exist_ok=True)
        bucket.get_object_to_file(obj.key, local_path)
        total += 1
        log.info("  downloaded: %s -> %s", obj.key, local_path)
    log.info("Downloaded %d objects from OSS prefix '%s'", total, oss_prefix)


def upload_to_oss(bucket: oss2.Bucket, local_path: str, oss_path: str):
    """Upload a local file to OSS, using multipart for files > 100 MB."""
    size = os.path.getsize(local_path)
    log.info("Uploading %s (%.1f MB) -> oss://%s", local_path, size / 1e6, oss_path)
    if size > 100 * 1024 * 1024:
        oss2.resumable_upload(bucket, oss_path, local_path,
                              multipart_threshold=100 * 1024 * 1024,
                              num_threads=4)
    else:
        bucket.put_object_from_file(oss_path, local_path)
    log.info("Upload complete: oss://%s", oss_path)


def run_hera_convert(src_dir: str, bag_path: str,
                     remap_file: str = "",
                     start_time: int = 0,
                     duration: int = 0) -> tuple:
    """Run hera-convert and return (success: bool, message: str)."""
    cmd = ["hera-convert", "-i", src_dir, "-o", bag_path, "-q"]
    if remap_file:
        cmd += ["-r", remap_file]
    if start_time > 0:
        cmd += ["-s", str(start_time)]
    if duration > 0:
        cmd += ["-t", str(duration)]

    log.info("Running: %s", " ".join(cmd))
    t0 = time.time()
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=82800,  # 23 hours — stay under FC async max
        )
    except subprocess.TimeoutExpired:
        return False, "hera-convert timed out"

    elapsed = time.time() - t0
    if result.returncode != 0:
        msg = f"hera-convert exited with code {result.returncode}: {result.stderr.strip()}"
        log.error(msg)
        return False, msg

    log.info("hera-convert finished in %.1fs", elapsed)
    return True, f"conversion completed in {elapsed:.1f}s"


def handle_convert(event: dict) -> dict:
    """Core conversion logic; called from the HTTP handler thread."""
    oss_endpoint   = event["oss_endpoint"]
    oss_bucket     = event["oss_bucket"]
    src_oss_prefix = event["src_oss_prefix"].rstrip("/") + "/"
    dst_oss_path   = event["dst_oss_path"]
    remap_oss_path = event.get("remap_oss_path", "")
    start_time     = int(event.get("start_time", 0))
    duration       = int(event.get("duration", 0))

    auth   = _make_auth()
    bucket = oss2.Bucket(auth, oss_endpoint, oss_bucket)

    with tempfile.TemporaryDirectory(prefix="hera_") as workdir:
        src_local  = os.path.join(workdir, "src")
        bag_local  = os.path.join(workdir, "output.bag")
        remap_local = ""

        # 1. Download hera data from OSS
        log.info("Step 1: downloading hera data from oss://%s%s", oss_bucket, src_oss_prefix)
        download_oss_prefix(bucket, src_oss_prefix, src_local)

        # 2. Optionally download remap config
        if remap_oss_path:
            remap_local = os.path.join(workdir, "remap.json")
            log.info("Step 1b: downloading remap config from oss://%s", remap_oss_path)
            bucket.get_object_to_file(remap_oss_path, remap_local)

        # 3. Run conversion
        log.info("Step 2: running hera-convert")
        ok, msg = run_hera_convert(src_local, bag_local, remap_local, start_time, duration)
        if not ok:
            return {"status": "error", "dst_oss_path": dst_oss_path, "message": msg}

        # 4. Upload ROS bag to OSS
        log.info("Step 3: uploading %s to oss://%s", bag_local, dst_oss_path)
        upload_to_oss(bucket, bag_local, dst_oss_path)

    return {"status": "success", "dst_oss_path": dst_oss_path, "message": msg}


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
        # FC invokes the function via POST /invoke
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length)

        try:
            event = json.loads(raw) if raw else {}
        except json.JSONDecodeError as e:
            self._send_json(400, {"status": "error", "message": f"Invalid JSON: {e}"})
            return

        required = {"oss_endpoint", "oss_bucket", "src_oss_prefix", "dst_oss_path"}
        missing = required - event.keys()
        if missing:
            self._send_json(400, {"status": "error",
                                  "message": f"Missing required fields: {missing}"})
            return

        try:
            result = handle_convert(event)
        except Exception as e:
            log.exception("Conversion failed with exception")
            result = {"status": "error", "message": str(e)}

        http_code = 200 if result["status"] == "success" else 500
        self._send_json(http_code, result)

    def do_GET(self):
        # Health check endpoint used by FC to verify the container is ready
        if self.path in ("/", "/health"):
            self._send_json(200, {"status": "ok"})
        else:
            self._send_json(404, {"status": "not found"})


def main():
    server = http.server.ThreadingHTTPServer(("0.0.0.0", PORT), RequestHandler)
    log.info("FC handler listening on port %d", PORT)
    server.serve_forever()


if __name__ == "__main__":
    main()
