"""Prints a join token for the integration tests, signed with `livekit-server --dev`'s key.

Usage: python3 test/dev_token.py <identity> <room>
"""

import base64
import hashlib
import hmac
import json
import sys
import time

_API_KEY = "devkey"
_API_SECRET = b"secret"


def main() -> None:
    identity, room = sys.argv[1:3]
    now = int(time.time())
    claims = {
        "iss": _API_KEY,
        "sub": identity,
        "iat": now,
        "nbf": now,
        "exp": now + 3600,
        "video": {"room": room, "roomJoin": True, "canPublish": True, "canSubscribe": True, "canPublishData": True},
    }
    signing = _b64(json.dumps({"alg": "HS256", "typ": "JWT"}).encode()) + "." + _b64(json.dumps(claims).encode())
    print(signing + "." + _b64(hmac.new(_API_SECRET, signing.encode(), hashlib.sha256).digest()))


def _b64(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode()


if __name__ == "__main__":
    main()
