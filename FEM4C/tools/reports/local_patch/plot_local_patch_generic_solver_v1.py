#!/usr/bin/env python3
import csv
import json
import math
import struct
import sys
import zlib
from pathlib import Path


FONT_5X7 = {
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "C": ["01111", "10000", "10000", "10000", "10000", "10000", "01111"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "G": ["01111", "10000", "10000", "10111", "10001", "10001", "01111"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "01010", "01010", "00100"],
    "X": ["10001", "01010", "00100", "00100", "00100", "01010", "10001"],
    "Y": ["10001", "01010", "00100", "00100", "00100", "00100", "00100"],
    "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    "3": ["11110", "00001", "00001", "01110", "00001", "00001", "11110"],
    "5": ["11111", "10000", "10000", "11110", "00001", "00001", "11110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "00001", "01110"],
    " ": ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
}


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def load_json(path: Path) -> dict:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"missing JSON file: {path}")
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON file {path}: {exc}")
    if not isinstance(data, dict):
        fail(f"{path} must contain a JSON object")
    return data


def png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + chunk_type
        + data
        + struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
    )


def write_png(path: Path, width: int, height: int, pixels: list[bytearray]) -> None:
    raw = b"".join(b"\x00" + bytes(row) for row in pixels)
    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(raw, level=9))
    png += png_chunk(b"IEND", b"")
    path.write_bytes(png)


def make_canvas(width: int, height: int, color: tuple[int, int, int]) -> list[bytearray]:
    row = bytearray(color * width)
    return [bytearray(row) for _ in range(height)]


def set_pixel(pixels: list[bytearray], width: int, height: int, x: int, y: int, color: tuple[int, int, int]) -> None:
    if 0 <= x < width and 0 <= y < height:
        idx = x * 3
        pixels[y][idx : idx + 3] = bytes(color)


def draw_rect(
    pixels: list[bytearray],
    width: int,
    height: int,
    x0: int,
    y0: int,
    x1: int,
    y1: int,
    color: tuple[int, int, int],
) -> None:
    xa, xb = sorted((x0, x1))
    ya, yb = sorted((y0, y1))
    for y in range(max(0, ya), min(height, yb)):
        row = pixels[y]
        for x in range(max(0, xa), min(width, xb)):
            idx = x * 3
            row[idx : idx + 3] = bytes(color)


def draw_line(
    pixels: list[bytearray],
    width: int,
    height: int,
    x0: int,
    y0: int,
    x1: int,
    y1: int,
    color: tuple[int, int, int],
) -> None:
    dx = abs(x1 - x0)
    dy = -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    x, y = x0, y0
    while True:
        set_pixel(pixels, width, height, x, y, color)
        if x == x1 and y == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x += sx
        if e2 <= dx:
            err += dx
            y += sy


def draw_text(
    pixels: list[bytearray],
    width: int,
    height: int,
    x: int,
    y: int,
    text: str,
    color: tuple[int, int, int],
    scale: int = 2,
) -> None:
    cursor_x = x
    for char in text.upper():
        glyph = FONT_5X7.get(char, FONT_5X7[" "])
        for row_idx, row_bits in enumerate(glyph):
            for col_idx, bit in enumerate(row_bits):
                if bit == "1":
                    draw_rect(
                        pixels,
                        width,
                        height,
                        cursor_x + col_idx * scale,
                        y + row_idx * scale,
                        cursor_x + (col_idx + 1) * scale,
                        y + (row_idx + 1) * scale,
                        color,
                    )
        cursor_x += 6 * scale


def color_map(value: float, vmin: float, vmax: float) -> tuple[int, int, int]:
    if vmax <= vmin:
        return (30, 80, 180)
    t = max(0.0, min(1.0, (value - vmin) / (vmax - vmin)))
    if t < 0.5:
        u = t / 0.5
        return (
            int(20 + 40 * u),
            int(70 + 160 * u),
            int(180 + 60 * u),
        )
    u = (t - 0.5) / 0.5
    return (
        int(60 + 195 * u),
        int(230 - 120 * u),
        int(240 - 220 * u),
    )


def plot_heatmap(out_dir: Path, mode: str, x_vals: list[float], y_vals: list[float], grid: list[list[float]]) -> None:
    width, height = 900, 620
    pixels = make_canvas(width, height, (255, 255, 255))
    left, top, plot_w, plot_h = 110, 90, 650, 420
    vmin = 0.0
    vmax = max(max(row) for row in grid)
    nx = len(x_vals)
    ny = len(y_vals)
    cell_w = plot_w / float(nx)
    cell_h = plot_h / float(ny)
    for iy in range(ny):
        for ix in range(nx):
            color = color_map(grid[iy][ix], vmin, vmax)
            x0 = int(left + ix * cell_w)
            x1 = int(left + (ix + 1) * cell_w + 1)
            y0 = int(top + (ny - 1 - iy) * cell_h)
            y1 = int(top + (ny - iy) * cell_h + 1)
            draw_rect(pixels, width, height, x0, y0, x1, y1, color)
    draw_line(pixels, width, height, left, top, left, top + plot_h, (0, 0, 0))
    draw_line(pixels, width, height, left, top + plot_h, left + plot_w, top + plot_h, (0, 0, 0))
    draw_text(pixels, width, height, 40, 20, f"PRESSURE HEATMAP {mode} PROXY", (0, 0, 0), 3)
    draw_text(pixels, width, height, 300, 545, "XT1 M", (0, 0, 0), 2)
    draw_text(pixels, width, height, 15, 280, "XT2 M", (0, 0, 0), 2)
    draw_text(pixels, width, height, 770, 120, "PRESSURE PA", (0, 0, 0), 2)
    for i in range(180):
        color = color_map(vmax * (1.0 - i / 179.0), vmin, vmax)
        draw_rect(pixels, width, height, 790, 150 + i, 820, 151 + i, color)
    write_png(out_dir / "pressure_field_heatmap.png", width, height, pixels)


def plot_cutline(out_dir: Path, mode: str, rows: list[dict[str, str]]) -> None:
    width, height = 900, 620
    pixels = make_canvas(width, height, (255, 255, 255))
    left, top, plot_w, plot_h = 100, 90, 700, 420
    center_y = min({float(row["x_t2_m"]) for row in rows}, key=lambda value: abs(value))
    cut_rows = sorted(
        (row for row in rows if abs(float(row["x_t2_m"]) - center_y) <= 1.0e-15),
        key=lambda row: float(row["x_t1_m"]),
    )
    x_vals = [float(row["x_t1_m"]) for row in cut_rows]
    y_vals = [float(row["pressure_pa"]) for row in cut_rows]
    xmin, xmax = min(x_vals), max(x_vals)
    ymax = max(y_vals) if y_vals else 1.0
    draw_line(pixels, width, height, left, top, left, top + plot_h, (0, 0, 0))
    draw_line(pixels, width, height, left, top + plot_h, left + plot_w, top + plot_h, (0, 0, 0))
    prev = None
    for x_val, y_val in zip(x_vals, y_vals):
        x = int(left + (x_val - xmin) / max(1.0e-16, xmax - xmin) * plot_w)
        y = int(top + plot_h - (y_val / max(1.0e-16, ymax)) * plot_h)
        if prev is not None:
            draw_line(pixels, width, height, prev[0], prev[1], x, y, (20, 90, 200))
        prev = (x, y)
    draw_text(pixels, width, height, 60, 20, f"CUTLINE XT1 {mode} PROXY", (0, 0, 0), 3)
    draw_text(pixels, width, height, 320, 545, "XT1 M", (0, 0, 0), 2)
    draw_text(pixels, width, height, 15, 250, "PRESSURE PA", (0, 0, 0), 2)
    write_png(out_dir / "pressure_field_centerline_t1.png", width, height, pixels)


def plot_reduced_summary(out_dir: Path, mode: str, response: dict) -> None:
    width, height = 900, 620
    pixels = make_canvas(width, height, (255, 255, 255))
    left, top, plot_w, plot_h = 90, 120, 720, 360
    metrics = [
        ("GAMMA", float(response["result"]["gamma_n"])),
        ("FN", float(response["result"]["fn_ref_n"])),
        ("PMAX", float(response["result"]["p_max_pa"])),
    ]
    vmax = max(value for _, value in metrics)
    bar_w = 140
    gap = 90
    colors = [(40, 170, 90), (240, 150, 30), (210, 60, 60)]
    draw_line(pixels, width, height, left, top, left, top + plot_h, (0, 0, 0))
    draw_line(pixels, width, height, left, top + plot_h, left + plot_w, top + plot_h, (0, 0, 0))
    for idx, ((label, value), color) in enumerate(zip(metrics, colors)):
        x0 = left + 60 + idx * (bar_w + gap)
        x1 = x0 + bar_w
        bar_h = int((value / max(1.0e-16, vmax)) * (plot_h - 30))
        y0 = top + plot_h - bar_h
        draw_rect(pixels, width, height, x0, y0, x1, top + plot_h, color)
        draw_text(pixels, width, height, x0 + 20, top + plot_h + 20, label, (0, 0, 0), 2)
    draw_text(pixels, width, height, 80, 20, f"REDUCED SUMMARY {mode} PROXY", (0, 0, 0), 3)
    draw_text(pixels, width, height, 180, 540, "PROXY FLAT PLANE STRUCTURED GRID", (0, 0, 0), 2)
    write_png(out_dir / "reduced_summary.png", width, height, pixels)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python3 scripts/plot_local_patch_generic_solver_v1.py <outdir>")

    out_dir = Path(sys.argv[1]).resolve()
    response = load_json(out_dir / "response.json")
    summary = load_json(out_dir / "local_patch_generic_summary.json")
    pressure_csv = out_dir / "pressure_field_grid.csv"
    if not pressure_csv.is_file():
        fail(f"missing pressure field CSV: {pressure_csv}")

    rows = list(csv.DictReader(pressure_csv.open(newline="")))
    if not rows:
        fail(f"pressure field CSV is empty: {pressure_csv}")

    x_vals = sorted({float(row["x_t1_m"]) for row in rows})
    y_vals = sorted({float(row["x_t2_m"]) for row in rows})
    if len(x_vals) <= 1 or len(y_vals) <= 1:
        fail("pressure field grid must have at least 2x2 cells")

    x_index = {value: idx for idx, value in enumerate(x_vals)}
    y_index = {value: idx for idx, value in enumerate(y_vals)}
    grid = [[0.0 for _ in x_vals] for _ in y_vals]
    for row in rows:
        x = float(row["x_t1_m"])
        y = float(row["x_t2_m"])
        p = float(row["pressure_pa"])
        if not math.isfinite(p):
            fail("pressure field contains non-finite values")
        grid[y_index[y]][x_index[x]] = p

    mode = str(summary["request_mode"]).upper()
    plot_heatmap(out_dir, mode, x_vals, y_vals, grid)
    plot_cutline(out_dir, mode, rows)
    plot_reduced_summary(out_dir, mode, response)

    print(f"PASS local_patch_generic_visualization_v1 mode={mode} png_count=3 out_dir={out_dir}")


if __name__ == "__main__":
    main()
