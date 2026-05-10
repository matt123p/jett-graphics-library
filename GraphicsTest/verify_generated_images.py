from __future__ import annotations

import csv
import sys
from pathlib import Path

from PIL import Image


def read_png_bit_depth(asset_path: Path) -> int:
    with asset_path.open("rb") as png_file:
        header = png_file.read(29)

    if len(header) < 29 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("invalid PNG signature")
    if header[12:16] != b"IHDR":
        raise ValueError("missing PNG IHDR chunk")

    return header[24]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_generated_images.py <manifest>")
        return 2

    manifest_path = Path(sys.argv[1]).resolve()
    if not manifest_path.is_file():
        print(f"manifest not found: {manifest_path}")
        return 2

    asset_root = manifest_path.parent
    failures: list[str] = []

    with manifest_path.open("r", newline="", encoding="utf-8") as manifest_file:
        reader = csv.DictReader(manifest_file, delimiter="\t")
        for row in reader:
            relative_path = row["path"]
            expected_format = row["format"]
            expected_mode = row["mode"]
            expected_bit_depth = int(row["bit_depth"])
            expected_width = int(row["width"])
            expected_height = int(row["height"])
            asset_path = (asset_root / relative_path).resolve()

            if not asset_path.is_file():
                failures.append(f"missing file: {relative_path}")
                continue

            try:
                with Image.open(asset_path) as image:
                    image.verify()
                with Image.open(asset_path) as image:
                    actual_format = image.format or ""
                    actual_mode = image.mode
                    actual_width, actual_height = image.size
            except Exception as exc:  # pragma: no cover - diagnostic path
                failures.append(f"invalid file: {relative_path}: {exc}")
                continue

            if actual_format.upper() != expected_format.upper():
                failures.append(
                    f"format mismatch for {relative_path}: expected {expected_format}, got {actual_format}"
                )
            if actual_mode.upper() != expected_mode.upper():
                failures.append(
                    f"mode mismatch for {relative_path}: expected {expected_mode}, got {actual_mode}"
                )
            if actual_format.upper() == "PNG":
                try:
                    actual_bit_depth = read_png_bit_depth(asset_path)
                except Exception as exc:  # pragma: no cover - diagnostic path
                    failures.append(f"unable to read PNG bit depth for {relative_path}: {exc}")
                else:
                    if actual_bit_depth != expected_bit_depth:
                        failures.append(
                            f"bit depth mismatch for {relative_path}: expected {expected_bit_depth}, got {actual_bit_depth}"
                        )
            if (actual_width, actual_height) != (expected_width, expected_height):
                failures.append(
                    f"size mismatch for {relative_path}: expected {expected_width}x{expected_height}, got {actual_width}x{actual_height}"
                )

    if failures:
        print("GENERATED IMAGE VALIDATION FAILED")
        for failure in failures:
            print(failure)
        return 1

    print(f"GENERATED IMAGE VALIDATION PASSED files={sum(1 for _ in manifest_path.open('r', encoding='utf-8')) - 1}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
