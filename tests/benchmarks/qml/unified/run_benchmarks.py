#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Run the unified QML benchmarks in every mode and emit a self-contained HTML report.

Usage:
    run_benchmarks.py [BUILD_DIR] [-o REPORT.html]

BUILD_DIR defaults to $QT_BUILD_DIR or ~/qt6dev-profile (the optimized build).
The report (charts as inline SVG, no external assets) defaults to ./benchmarks.html.
"""

import argparse
import csv
import math
import os
import shutil
import subprocess
import sys
import tempfile
from collections import namedtuple
from datetime import datetime, timezone
from html import escape
from pathlib import Path

# One measured cell: the per-iteration minimum (nanoseconds) and how many iterations
# QTest ran to get it. iters is 1 for the cold compile rounds and for run 1st.
Measure = namedtuple("Measure", ["ns", "iters"])

# (label, executable, colour) — one mode per disk-cache regime / bundle kind.
MODES = [
    ("source", "tst_unified_source", "#d9534f"),
    ("qmlc", "tst_unified_qmlc", "#f0ad4e"),
    ("aot", "tst_unified_aot", "#5cb85c"),
]
# Phases, in the order the executables report them. "runfirst"/"run" are the cold
# first invocation and the warm steady state of the document's central operation.
PHASES = ["compile", "create", "runfirst", "run"]
PHASE_LABEL = {"compile": "compile", "create": "create", "runfirst": "run 1st", "run": "run more"}
PHASE_BLURB = {
    "compile": "cold load into a fresh engine (compile from source / read .qmlc / map AOT unit)",
    "create": "instantiate the root object on a pre-heated engine",
    "runfirst": "one cold invocation of the central operation (populates the inline-cache lookups)",
    "run": "the warm steady state of that operation (where AOT-compiled bindings can win)",
}

# QBENCHMARK reports WalltimeMilliseconds; QBENCHMARK_ONCE (run 1st) reports the same.
# Normalise everything to nanoseconds via the metric column.
METRIC_TO_NS = {"WalltimeNanoseconds": 1.0, "WalltimeMilliseconds": 1e6}
EXE_SUBDIR = "qtdeclarative/tests/benchmarks/qml/unified"


def parse_args():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("build_dir", nargs="?",
                   default=os.environ.get("QT_BUILD_DIR",
                                          str(Path.home() / "qt6dev-profile")),
                   help="Qt build directory (default: $QT_BUILD_DIR or ~/qt6dev-profile)")
    p.add_argument("-o", "--output", default="benchmarks.html", help="HTML report path")
    p.add_argument("-n", "--reps", type=int, default=3,
                   help="repetitions per mode; the minimum per cell is kept (default 3)")
    p.add_argument("--core", type=int, default=None,
                   help="pin the benchmark process to this CPU core (default: auto)")
    p.add_argument("--minimum-value", type=int, default=1,
                   help="QTest -minimumvalue in ms: a warm 'run' batch grows until it "
                        "takes this long, bounding the iteration count (default 1)")
    return p.parse_args()


def pin_prefix(core):
    """Command prefix to pin a run to one core (taskset), to steady the clock."""
    if not shutil.which("taskset"):
        return [], None
    if core is None:
        core = 1 if (os.cpu_count() or 1) > 1 else 0
    return ["taskset", "-c", str(core)], core


def cpu_governor():
    try:
        return Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor").read_text().strip()
    except OSError:
        return None


def run_mode(build_dir, exe, prefix, qtest_args):
    """Run one mode executable once, return {(phase, doc): Measure}."""
    path = Path(build_dir) / EXE_SUBDIR / exe
    if not path.exists():
        sys.exit(f"error: executable not found: {path}\n"
                 f"build it first, e.g.  ninja {exe}")
    fd, csv_path = tempfile.mkstemp(suffix=".csv")
    os.close(fd)
    proc = subprocess.run(prefix + [str(path), *qtest_args, "-o", f"{csv_path},csv"],
                          stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if proc.returncode != 0:
        print(f"    warning: {exe} exited with {proc.returncode} "
              f"(some rows may be missing)", file=sys.stderr)
    results = {}
    # CSV columns: function,tag,metric,value_per_iteration,total,stddev,iterations
    with open(csv_path, newline="") as f:
        for row in csv.reader(f):
            if len(row) < 7 or row[0] not in PHASES:
                continue
            scale = METRIC_TO_NS.get(row[2])
            if scale is None:
                continue
            try:
                results[(row[0], row[1])] = Measure(float(row[3]) * scale, int(float(row[6])))
            except ValueError:
                pass
    os.unlink(csv_path)
    return results


def collect(build_dir, reps, prefix, qtest_args):
    """Return (data, docs, groups) where data[doc][phase][mode_label] = Measure.

    Modes are interleaved across `reps` repetitions and the minimum per cell is
    kept. The minimum is the right estimator here: each mode is a separate process
    (QML_DISK_CACHE is per-process), so runs land in different CPU power states;
    taking the best run per mode removes that bimodal scheduling noise instead of
    averaging it in, and matches the per-iteration min the harness already uses.
    """
    best = {label: {} for label, _, _ in MODES}
    for rep in range(reps):
        for label, exe, _ in MODES:
            print(f"  rep {rep + 1}/{reps}: {exe} ...", flush=True)
            for key, measure in run_mode(build_dir, exe, prefix, qtest_args).items():
                cur = best[label].get(key)
                if cur is None or measure.ns < cur.ns:
                    best[label][key] = measure
    docs = sorted({doc for m in best.values() for (_, doc) in m})
    data = {}
    for doc in docs:
        data[doc] = {phase: {label: best[label].get((phase, doc)) for label, _, _ in MODES}
                     for phase in PHASES}
    groups = sorted({doc.rsplit("/", 1)[0] for doc in docs})
    return data, docs, groups


def fmt(ns):
    if ns is None:
        return "—"
    if ns < 1e3:
        return f"{ns:.0f}&nbsp;ns"
    if ns < 1e6:
        return f"{ns / 1e3:.1f}&nbsp;µs"
    return f"{ns / 1e6:.2f}&nbsp;ms"


def geomean(measures):
    vals = [m.ns for m in measures if m and m.ns > 0]
    if not vals:
        return None
    return math.exp(sum(math.log(v) for v in vals) / len(vals))


def legend():
    items = "".join(
        f'<span class="key"><span class="swatch" style="background:{c}"></span>{escape(l)}</span>'
        for l, _, c in MODES)
    return f'<div class="legend">{items}</div>'


def build_html(build_dir, data, docs, env):
    colour = {label: c for label, _, c in MODES}
    parts = []
    parts.append(f"""<!doctype html><html><head><meta charset="utf-8">
<title>Unified QML benchmarks</title>
<style>
 body {{ font-family: sans-serif; margin: 24px; color: #1a1a1a; }}
 h1 {{ font-size: 22px; }} h2 {{ margin-top: 34px; border-bottom: 2px solid #eee; padding-bottom: 4px; }}
 .meta {{ color: #555; font-size: 13px; }}
 .legend {{ margin: 10px 0; }} .key {{ margin-right: 18px; font-size: 13px; }}
 .swatch {{ display: inline-block; width: 12px; height: 12px; border-radius: 2px;
            margin-right: 5px; vertical-align: middle; }}
 table {{ border-collapse: collapse; font-size: 12px; margin-top: 8px; width: 100%; }}
 th, td {{ padding: 3px 8px; text-align: right; border-bottom: 1px solid #eee; white-space: nowrap; }}
 th {{ background: #fafafa; position: sticky; top: 0; }}
 td.doc {{ text-align: left; font-family: monospace; }}
 tr.group td {{ background: #f4f6f8; font-weight: bold; text-align: left; font-family: monospace; }}
 .best {{ font-weight: bold; color: #2e7d32; }}
 .phasehdr {{ border-left: 2px solid #ddd; }}
 .gtable th {{ text-align: center; }}
 .gtable td.cell {{ padding: 4px 8px; min-width: 190px; vertical-align: middle;
                    border-left: 1px solid #eee; }}
 .gtable td.cell.empty {{ color: #bbb; text-align: center; }}
 .bar {{ position: relative; height: 14px; margin: 2px 0; background: #f3f3f3; border-radius: 2px; }}
 .fill {{ position: absolute; left: 0; top: 0; height: 100%; border-radius: 2px; opacity: 0.85; }}
 .blbl {{ position: relative; font-size: 10px; padding-left: 5px; line-height: 14px;
          color: #1a1a1a; white-space: nowrap; }}
 .blbl.best {{ font-weight: bold; color: #2e7d32; }}
 .blbl .it {{ color: #999; }}
</style></head><body>""")

    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    parts.append(f"<h1>Unified QML benchmarks</h1>")
    pin = f"core {env['core']}" if env.get("core") is not None else "unpinned"
    gov = env.get("governor") or "unknown"
    parts.append(f'<p class="meta">Generated {now} · build <code>{escape(str(build_dir))}</code> · '
                 f'offscreen platform · {len(docs)} documents × {len(MODES)} modes × '
                 f'{len(PHASES)} phases · min of {env["reps"]} interleaved reps · '
                 f'pinned to {pin} · CPU governor: {escape(gov)} · '
                 f'QTest args: <code>{escape(env.get("qtest", ""))}</code></p>')
    parts.append("<p class=\"meta\">Modes: <b>source</b> = compile from source (disk cache off) · "
                 "<b>qmlc</b> = load run-time .qmlc · <b>aot</b> = load unit baked into the binary. "
                 "Lower is better; each value is the minimum over the timed iterations.</p>")
    parts.append(legend())

    def win_counts(phase):
        counts = {label: 0 for label, _, _ in MODES}
        for doc in docs:
            present = [(label, data[doc][phase][label].ns)
                       for label, _, _ in MODES if data[doc][phase][label]]
            if not present:
                continue
            counts[min(present, key=lambda lv: lv[1])[0]] += 1
        return counts

    # Headline summary: for each phase, how many documents each mode was fastest in.
    # Bars are scaled to the number of documents measured in that phase, so a bar that
    # fills the width means that mode won every document. This is the only meaningful
    # cross-document comparison (the documents themselves measure unrelated things).
    parts.append("<h2>Fastest variant (documents won per phase)</h2>")
    parts.append('<p class="meta">For each phase, the number of documents each mode was the '
                 'fastest in. Bars are scaled to the documents measured in that phase — a '
                 'full-width bar means that mode won them all.</p>')
    head = "".join(f"<th>{PHASE_LABEL[p]}</th>" for p in PHASES)
    parts.append(f"<table class='gtable'><tr>{head}</tr><tr>")
    for phase in PHASES:
        counts = win_counts(phase)
        total = sum(counts.values())
        winner = max(counts.values(), default=0)
        bars = []
        for label, _, _ in MODES:
            n = counts[label]
            width = (n / total * 100.0) if total else 0.0
            cls = " best" if n and n == winner else ""
            bars.append(
                f"<div class='bar'><div class='fill' "
                f"style='width:{width:.1f}%;background:{colour[label]}'></div>"
                f"<span class='blbl{cls}'>{escape(label)}: {n}</span></div>")
        parts.append(f"<td class='cell'>{''.join(bars)}</td>")
    parts.append("</tr></table>")

    # Summary table: geometric mean per phase per mode.
    parts.append("<h2>Summary (geometric mean over all documents)</h2>")
    parts.append("<table><tr><th>phase</th>" +
                 "".join(f"<th>{escape(l)}</th>" for l, _, _ in MODES) +
                 "<th>what it measures</th></tr>")
    for phase in PHASES:
        cells = []
        gm = {label: geomean([data[d][phase][label] for d in docs]) for label, _, _ in MODES}
        best = min((v for v in gm.values() if v), default=None)
        for label, _, _ in MODES:
            cls = ' class="best"' if gm[label] == best else ""
            cells.append(f"<td{cls}>{fmt(gm[label])}</td>")
        parts.append(f"<tr><td class='doc'>{PHASE_LABEL[phase]}</td>{''.join(cells)}"
                     f"<td class='doc' style='color:#666'>{escape(PHASE_BLURB[phase])}</td></tr>")
    parts.append("</table>")

    # Detail table: one graph per document x phase. Each cell holds one bar per mode,
    # normalized PER CELL — the slowest mode in that cell fills the full width — so a
    # fast document and a slow one are equally legible. Cells are therefore NOT
    # comparable to each other; only the printed values are.
    parts.append("<h2>All documents</h2>")
    parts.append('<p class="meta">One graph per document × phase; a bar per mode (labelled), scaled '
                 'so the slowest mode in that cell fills the width — cells are not comparable to '
                 'each other, only the printed values are. The label is the per-iteration minimum '
                 'and <code>×N</code> iterations.</p>')

    def cell(doc, phase):
        vals = data[doc][phase]
        present = [m.ns for m in vals.values() if m and m.ns > 0]
        if not present:
            return "<td class='cell empty'>—</td>"
        cellmax = max(present)
        fastest = min(present)
        bars = []
        for label, _, _ in MODES:
            m = vals[label]
            if not m:
                bars.append("<div class='bar'><span class='blbl'>—</span></div>")
                continue
            width = m.ns / cellmax * 100.0
            cls = " best" if m.ns == fastest else ""
            bars.append(
                f"<div class='bar'><div class='fill' "
                f"style='width:{width:.1f}%;background:{colour[label]}'></div>"
                f"<span class='blbl{cls}'>{escape(label)} {fmt(m.ns)} "
                f"<span class='it'>×{m.iters}</span></span></div>")
        return f"<td class='cell'>{''.join(bars)}</td>"

    dhead = "".join(f"<th>{PHASE_LABEL[p]}</th>" for p in PHASES)
    parts.append(f"<table class='gtable'><tr><th class='doc'>document</th>{dhead}</tr>")
    cur_group = None
    for doc in docs:
        g = doc.rsplit("/", 1)[0]
        name = doc.rsplit("/", 1)[1]
        if g != cur_group:
            cur_group = g
            parts.append(f"<tr class='group'><td colspan='{1 + len(PHASES)}'>"
                         f"{escape(g)}</td></tr>")
        cells = "".join(cell(doc, p) for p in PHASES)
        parts.append(f"<tr><td class='doc'>{escape(name)}</td>{cells}</tr>")
    parts.append("</table>")

    parts.append("</body></html>")
    return "\n".join(parts)


def main():
    args = parse_args()
    prefix, core = pin_prefix(args.core)
    gov = cpu_governor()
    print(f"build dir: {args.build_dir}")
    qtest_args = ["-minimumvalue", str(args.minimum_value)]
    print(f"reps: {args.reps} (min per cell)  pin: "
          f"{'core ' + str(core) if core is not None else 'none (taskset missing)'}  "
          f"qtest: {' '.join(qtest_args)}")
    if gov and gov != "performance":
        print(f"note: CPU governor is '{gov}', not 'performance'. For steadier numbers run:\n"
              f"      sudo cpupower frequency-set -g performance   "
              f"(and consider disabling turbo)", file=sys.stderr)
    env = {"reps": args.reps, "core": core, "governor": gov, "qtest": " ".join(qtest_args)}
    data, docs, groups = collect(args.build_dir, args.reps, prefix, qtest_args)
    if not docs:
        sys.exit("error: no benchmark results parsed")
    html_text = build_html(args.build_dir, data, docs, env)
    Path(args.output).write_text(html_text, encoding="utf-8")
    print(f"wrote {args.output}  ({len(docs)} documents, {len(groups)} groups)")


if __name__ == "__main__":
    main()
