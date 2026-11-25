#!/usr/bin/env python3
"""Editorial sample-boundary checks and FFmpeg CLI measurements.

Synthetic fixtures only. Does not modify the 2026-11-20 MXF round-trip
directory and does not install packages.
"""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path("/home/editry/development/sise-editor")
FIX = ROOT / "validation-fixtures" / "audio-sample-boundary-2026-11-20"
SRC = FIX / "src"
OUT = FIX / "out"
LOG = FIX / "logs"
SR = 48000
RATES = [
    ("24", 24, 1),
    ("25", 25, 1),
    ("24000_1001", 24000, 1001),
    ("30000_1001", 30000, 1001),
    ("30", 30, 1),
]


def boundary(n: int, num: int, den: int, sr: int = SR) -> int:
    if n < 0:
        raise ValueError("editorial sample_boundary is not defined for n < 0")
    return (n * sr * den) // num


def ceil_boundary(n: int, num: int, den: int, sr: int = SR) -> int:
    product = n * sr * den
    return (product + num - 1) // num


def nearest_half_up(n: int, num: int, den: int, sr: int = SR) -> int:
    product = n * sr * den
    return (product + num // 2) // num


def frame_count(start: int, num: int, den: int, sr: int = SR) -> int:
    return boundary(start + 1, num, den, sr) - boundary(start, num, den, sr)


def write_pcm(path: Path, samples: int, channels: int = 2) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    frame = b"\x00\x00\x00" * channels
    with path.open("wb") as handle:
        handle.write(frame * samples)


def run(cmd: list[str], log_name: str) -> subprocess.CompletedProcess[str]:
    LOG.mkdir(parents=True, exist_ok=True)
    text_path = LOG / log_name
    proc = subprocess.run(cmd, text=True, capture_output=True)
    text_path.write_text(
        "$ " + " ".join(cmd) + f"\nexit={proc.returncode}\n"
        + "----- stdout -----\n" + proc.stdout
        + "----- stderr -----\n" + proc.stderr
    )
    return proc


def probe(path: Path, tag: str) -> dict:
    cmd = [
        "ffprobe", "-hide_banner", "-v", "error",
        "-show_format", "-show_streams",
        "-of", "json", str(path),
    ]
    proc = run(cmd, f"{tag}.ffprobe.json.log")
    data = json.loads(proc.stdout) if proc.returncode == 0 and proc.stdout.strip() else {}
    (LOG / f"{tag}.ffprobe.json").write_text(proc.stdout)
    packets = run(
        [
            "ffprobe", "-hide_banner", "-v", "error",
            "-select_streams", "a",
            "-show_entries", "packet=pts,duration,size,flags",
            "-of", "json", str(path),
        ],
        f"{tag}.packets.json.log",
    )
    pdata = json.loads(packets.stdout) if packets.returncode == 0 and packets.stdout.strip() else {}
    (LOG / f"{tag}.packets.json").write_text(packets.stdout)
    return {"streams": data, "packets": pdata, "probe_exit": proc.returncode}


def audio_stream(info: dict) -> dict | None:
    for stream in info.get("streams", {}).get("streams", []):
        if stream.get("codec_type") == "audio":
            return stream
    return None


def video_stream(info: dict) -> dict | None:
    for stream in info.get("streams", {}).get("streams", []):
        if stream.get("codec_type") == "video":
            return stream
    return None


def packet_durations(info: dict) -> list[int]:
    rows = []
    for packet in info.get("packets", {}).get("packets", []):
        if "duration" not in packet:
            continue
        rows.append(int(packet["duration"]))
    return rows


def mux_mxf(tag: str, rate: str, frames: int, pcm: Path, video_codec: list[str]) -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    dest = OUT / f"{tag}.mxf"
    cmd = [
        "ffmpeg", "-hide_banner", "-y",
        "-f", "lavfi", "-i", f"color=c=black:s=32x32:r={rate},format=yuv422p",
        "-f", "s24le", "-ar", "48000", "-ac", "2", "-i", str(pcm),
        "-map", "0:v:0", "-map", "1:a:0",
        "-frames:v", str(frames),
        *video_codec,
        "-c:a", "pcm_s24le",
        "-f", "mxf",
        str(dest),
    ]
    proc = run(cmd, f"{tag}.ffmpeg.log")
    if proc.returncode != 0 or not dest.exists():
        raise RuntimeError(f"mux failed for {tag}: exit {proc.returncode}")
    return dest


def arithmetic_report(lines: list[str]) -> None:
    lines.append("== arithmetic: existing sampleIndex / candidate sample_boundary ==")
    lines.append("sample_boundary(n) = floor(n * 48000 * den / num) for n >= 0")
    lines.append("segment(start, end) = sample_boundary(end) - sample_boundary(start)")
    for name, num, den in RATES:
        lines.append(f"-- rate {name} {num}/{den}")
        points = [0, 1, 2, 5, 8, 16, 24, 40, 100, 1000, 107892, 200000]
        prev = 0
        running = 0
        mismatch = 0
        for n in points:
            pos = boundary(n, num, den)
            # running sum of per-frame counts up to n, checked only when we walk
            lines.append(f"boundary({n})={pos}")
        # telescoping identity over a long span, by walking frames in chunks
        # of the closed form: sum of frame counts equals boundary(N)
        n_long = 200000
        walked = 0
        # walk in python; 200k iterations is small
        acc = 0
        for frame in range(n_long):
            acc += frame_count(frame, num, den)
        end = boundary(n_long, num, den)
        lines.append(f"walk_sum(0,{n_long})={acc} boundary={end} match={acc == end}")
        # adjacent eight-frame segments
        lines.append("adjacent eight-frame segments from frame 0:")
        cursor = 0
        total = 0
        for index in range(8):
            count = boundary(cursor + 8, num, den) - boundary(cursor, num, den)
            lines.append(
                f"  [{cursor},{cursor + 8}) samples={count} "
                f"abs_start={boundary(cursor, num, den)} abs_end={boundary(cursor + 8, num, den)}"
            )
            total += count
            cursor += 8
        union = boundary(64, num, den) - boundary(0, num, den)
        lines.append(f"  sum_of_segments={total} union[0,64)={union} tile={total == union}")
        # non-zero starts
        for start in (0, 8, 100, 1000):
            count = boundary(start + 8, num, den) - boundary(start, num, den)
            origin = boundary(8, num, den)
            lines.append(
                f"  span[{start},{start + 8})={count} origin_8={origin} "
                f"equal_to_origin={count == origin}"
            )
        # multiple edit boundaries
        cuts = [0, 8, 24, 25, 40]
        piece_sum = 0
        for a, b in zip(cuts, cuts[1:]):
            piece = boundary(b, num, den) - boundary(a, num, den)
            piece_sum += piece
            lines.append(f"  edit[{a},{b})={piece}")
        lines.append(
            f"  edit_sum={piece_sum} union={boundary(cuts[-1], num, den)} "
            f"tile={piece_sum == boundary(cuts[-1], num, den)}"
        )
        # first eight frame sizes at 30000/1001 are the published pattern
        if num == 30000 and den == 1001:
            sizes = [frame_count(f, num, den) for f in range(8)]
            lines.append(f"first_8_frame_sizes={sizes} sum={sum(sizes)}")
            lines.append(f"boundary(8)={boundary(8, num, den)} expected_contract=12812")
    lines.append("== rounding alternatives at 30000/1001, 48000 ==")
    num, den = 30000, 1001
    for n in (0, 1, 5, 8, 10, 16, 40, 1000, 107892):
        exact_num = n * SR * den
        lines.append(
            f"n={n} floor={boundary(n, num, den)} ceil={ceil_boundary(n, num, den)} "
            f"nearest_half_up={nearest_half_up(n, num, den)} "
            f"exact={exact_num}/{num}"
        )
    # unbounded drift of rounding each frame to 1602
    drift_frames = 1000
    always_1602 = 1602 * drift_frames
    floor_pos = boundary(drift_frames, num, den)
    lines.append(
        f"constant_1602_over_{drift_frames}={always_1602} "
        f"floor={floor_pos} drift={always_1602 - floor_pos}"
    )
    lines.append("== negative index ==")
    lines.append("editorial rule rejects a negative stored position; sample_boundary(n) is specified for n >= 0 only")
    product = -1 * SR * 1001
    lines.append(
        f"illustration_only F=-1 product={product} "
        f"python_floor_div={product // 30000} "
        f"toward_zero_trunc={int(product / 30000)} "
        f"these_are_not_a_contract"
    )


def main() -> int:
    SRC.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    LOG.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    arithmetic_report(lines)
    results: list[str] = []
    mpeg = ["-c:v", "mpeg2video", "-b:v", "80k", "-pix_fmt", "yuv422p"]
    prores = ["-c:v", "prores_ks", "-profile:v", "hq", "-pix_fmt", "yuv422p10le"]

    def measure(tag: str, num: int, den: int, frames: int, samples: int, codec: list[str], rate_token: str) -> None:
        pcm = SRC / f"{tag}.s24le"
        write_pcm(pcm, samples)
        dest = mux_mxf(tag, rate_token, frames, pcm, codec)
        info = probe(dest, tag)
        a = audio_stream(info) or {}
        v = video_stream(info) or {}
        durations = packet_durations(info)
        duration_sum = sum(durations)
        nb = a.get("duration_ts")
        # ffprobe may report nb_samples
        reported = a.get("nb_samples", nb)
        vframes = v.get("nb_frames") or v.get("nb_read_frames")
        expect = boundary(frames, num, den)
        results.append(
            f"{tag} frames={frames} supplied={samples} expect_boundary={expect} "
            f"audio_nb_samples={a.get('nb_samples')} duration_ts={a.get('duration_ts')} "
            f"video_nb_frames={vframes} r_frame_rate={v.get('r_frame_rate')} "
            f"audio_packets={len(durations)} packet_duration_sum={duration_sum} "
            f"first_packets={durations[:12]}"
        )
        if num == 30000 and frames >= 16 and durations:
            lines_local = []
            acc = 0
            max_abs = 0
            growing = False
            prev_err = 0
            for index, dur in enumerate(durations):
                acc += dur
                want = boundary(index + 1, num, den) - boundary(0, num, den)
                err = acc - want
                if abs(err) > max_abs:
                    max_abs = abs(err)
                if abs(err) > abs(prev_err):
                    growing = True
                prev_err = err
            results.append(
                f"{tag} cumulative_vs_floor max_abs_error={max_abs} "
                f"error_increased_somewhere={growing} final_acc={acc} "
                f"final_floor={boundary(len(durations), num, den)}"
            )
            for start in range(0, min(len(durations), 40), 8):
                chunk = durations[start:start + 8]
                if len(chunk) < 8:
                    break
                got = sum(chunk)
                want = boundary(start + 8, num, den) - boundary(start, num, den)
                results.append(
                    f"{tag} segment[{start},{start + 8}) ffmpeg={got} contract={want} "
                    f"abs_contract_start={boundary(start, num, den)} "
                    f"abs_ffmpeg_start={sum(durations[:start])}"
                )

    jobs = []
    for name, num, den in RATES:
        token = f"{num}/{den}" if den != 1 else str(num)
        frames = 8
        samples = boundary(frames, num, den)
        jobs.append((f"mpeg_{name}_8f", num, den, frames, samples, mpeg, token))
    # critical 29.97 reproductions
    jobs.append(("prores_30000_1001_8f_supplied12812", 30000, 1001, 8, 12812, prores, "30000/1001"))
    jobs.append(("prores_30000_1001_8f_supplied12813", 30000, 1001, 8, 12813, prores, "30000/1001"))
    jobs.append(("mpeg_30000_1001_40f", 30000, 1001, 40, boundary(40, 30000, 1001), mpeg, "30000/1001"))
    jobs.append(("mpeg_30000_1001_600f", 30000, 1001, 600, boundary(600, 30000, 1001), mpeg, "30000/1001"))
    # integer rates, longer, to show exact continuity
    jobs.append(("mpeg_24_40f", 24, 1, 40, boundary(40, 24, 1), mpeg, "24"))
    jobs.append(("mpeg_24000_1001_40f", 24000, 1001, 40, boundary(40, 24000, 1001), mpeg, "24000/1001"))

    for job in jobs:
        try:
            measure(*job)
        except Exception as exc:  # noqa: BLE001 — record the real failure
            results.append(f"{job[0]} EXCEPTION {exc}")

    # audio-only and video-only controls
    pcm_only = SRC / "audio_only_12812.s24le"
    write_pcm(pcm_only, 12812)
    wav = OUT / "audio_only_12812.wav"
    proc = run(
        [
            "ffmpeg", "-hide_banner", "-y",
            "-f", "s24le", "-ar", "48000", "-ac", "2", "-i", str(pcm_only),
            "-c:a", "pcm_s24le", str(wav),
        ],
        "audio_only_12812.ffmpeg.log",
    )
    if proc.returncode == 0:
        info = probe(wav, "audio_only_12812")
        a = audio_stream(info) or {}
        results.append(
            f"audio_only_wav supplied=12812 nb_samples={a.get('nb_samples')} "
            f"duration_ts={a.get('duration_ts')} video_present={video_stream(info) is not None}"
        )
    else:
        results.append(f"audio_only_wav FAIL exit={proc.returncode}")

    vonly = OUT / "video_only_8f_30000_1001.mxf"
    proc = run(
        [
            "ffmpeg", "-hide_banner", "-y",
            "-f", "lavfi", "-i", "color=c=black:s=32x32:r=30000/1001,format=yuv422p",
            "-frames:v", "8",
            "-c:v", "mpeg2video", "-b:v", "80k", "-pix_fmt", "yuv422p",
            "-an", "-f", "mxf", str(vonly),
        ],
        "video_only_8f.ffmpeg.log",
    )
    if proc.returncode == 0:
        info = probe(vonly, "video_only_8f")
        v = video_stream(info) or {}
        results.append(
            f"video_only_mxf video_nb_frames={v.get('nb_frames')} "
            f"audio_present={audio_stream(info) is not None}"
        )
    else:
        results.append(f"video_only_mxf FAIL exit={proc.returncode}")

    text = "\n".join(lines + ["== ffmpeg measurements =="] + results) + "\n"
    (LOG / "summary.txt").write_text(text)
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
