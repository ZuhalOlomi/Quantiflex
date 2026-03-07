#!/usr/bin/env bash
# Simple development runner for the Quantiflex project
# Starts backend (Flask) on port 5001 and a static HTTP server for the frontend on port 8000.
# Usage: ./run-dev.sh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BACKEND_DIR="$ROOT_DIR/backend"
FRONTEND_DIR="$ROOT_DIR/website"

# Activate venv
if [ -f "$BACKEND_DIR/.venv/bin/activate" ]; then
  # shellcheck disable=SC1091
  source "$BACKEND_DIR/.venv/bin/activate"
else
  echo "Warning: virtualenv not found at $BACKEND_DIR/.venv — make sure dependencies are installed"
fi

# Start backend on port 5001 in background
echo "Starting backend on http://localhost:5001"
python - <<'PY' &
import os
from app import app
app.run(host='0.0.0.0', port=5001, debug=True)
PY
BACKEND_PID=$!

# Start static server for frontend on port 8000
echo "Starting frontend on http://localhost:8000"
(
  cd "$FRONTEND_DIR"
  python3 -m http.server 8000
) &
FRONTEND_PID=$!

# Echo PIDs and wait
echo "Backend PID: $BACKEND_PID"
echo "Frontend PID: $FRONTEND_PID"

echo "Press Ctrl+C to stop both servers"

trap "echo 'Stopping...'; kill $BACKEND_PID $FRONTEND_PID; exit 0" SIGINT SIGTERM

# wait for children
wait
