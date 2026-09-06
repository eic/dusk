#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <output-dir>" >&2
  exit 1
fi

OUT_DIR="$1"
ZIP_URL="https://raw.githubusercontent.com/usnistgov/SFA/48bcd4ccef07db7b903baaefaf6a2eb427d02b05/Release/NIST-PMI-STEP-Files.zip"
TMP_DIR="$(mktemp -d)"
ZIP_FILE="${TMP_DIR}/NIST-PMI-STEP-Files.zip"
EXTRACT_DIR="${TMP_DIR}/extracted"

cleanup() {
  chmod -R u+rwX "${TMP_DIR}" 2>/dev/null || true
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

mkdir -p "${OUT_DIR}"

echo "Downloading NIST CTC archive..."
curl -L --fail --max-time 300 --retry 3 -o "${ZIP_FILE}" "${ZIP_URL}"

if ! unzip -t "${ZIP_FILE}" >/dev/null 2>&1; then
  echo "ERROR: Invalid ZIP archive downloaded from ${ZIP_URL}" >&2
  exit 1
fi

mkdir -p "${EXTRACT_DIR}"
unzip -q "${ZIP_FILE}" -d "${EXTRACT_DIR}"
chmod -R u+rwX "${EXTRACT_DIR}" 2>/dev/null || true

AP203_DIR=""
while IFS= read -r -d '' dir; do
  base="$(basename "${dir}")"
  lower="${base,,}"
  if [[ "${lower}" == *"ap203"* ]]; then
    if compgen -G "${dir}/*.stp" >/dev/null 2>&1 || compgen -G "${dir}/*.step" >/dev/null 2>&1; then
      AP203_DIR="${dir}"
      break
    fi
  fi
done < <(find "${EXTRACT_DIR}" -type d -print0 | sort -z)

if [[ -z "${AP203_DIR}" ]]; then
  echo "ERROR: Could not locate AP203 STEP directory in archive." >&2
  exit 1
fi

mapfile -t STEP_FILES < <(find "${AP203_DIR}" -maxdepth 1 \( -name "*.stp" -o -name "*.step" \) | sort)
if [[ "${#STEP_FILES[@]}" -lt 11 ]]; then
  echo "ERROR: Expected at least 11 STEP files, found ${#STEP_FILES[@]}." >&2
  exit 1
fi

for i in $(seq 0 10); do
  idx="$(printf '%02d' $((i + 1)))"
  cp "${STEP_FILES[$i]}" "${OUT_DIR}/nist-ctc-${idx}.step"
done

echo "Fetched 11 NIST CTC STEP files into ${OUT_DIR}"
