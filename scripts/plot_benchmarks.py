"""
Generate publication-ready benchmark charts for the diploma thesis.

Input CSV files:
    bench1_neural_vs_tfidf_quality.csv
    bench2_neural_vs_tfidf_latency.csv
    bench3_word_vs_bpe_tokenization.csv
    bench4_tokenizer_oov_coverage.csv
    bench5_multithread_training.csv
    bench6_training_loss_curve.csv

Usage:
    python scripts/plot_benchmarks.py [csv_dir] [out_dir]

Defaults:
    csv_dir = benchmarks
    out_dir = csv_dir
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

CSV_DIR = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("benchmarks")
OUT_DIR = Path(sys.argv[2]) if len(sys.argv) > 2 else CSV_DIR
OUT_DIR.mkdir(parents=True, exist_ok=True)

NEURAL_BLUE = "#2F5DA8"
TFIDF_ORANGE = "#D97A30"
GRID = "#D9D9D9"
TEXT = "#222222"
MUTED = "#666666"
LOSS_BLUE = NEURAL_BLUE

plt.rcParams.update({
    "figure.dpi": 120,
    "savefig.dpi": 300,
    "font.family": "DejaVu Sans",
    "font.size": 10,
    "axes.titlesize": 12,
    "axes.labelsize": 10.5,
    "axes.edgecolor": "#333333",
    "axes.linewidth": 0.8,
    "xtick.labelsize": 9.5,
    "ytick.labelsize": 9.5,
    "legend.fontsize": 9.5,
    "axes.spines.top": False,
    "axes.spines.right": False,
})


def read_csv(name: str) -> pd.DataFrame:
    path = CSV_DIR / name
    if not path.exists():
        raise FileNotFoundError(f"Missing required CSV file: {path}")
    return pd.read_csv(path)


def save_figure(fig: plt.Figure, stem: str) -> None:
    png_path = OUT_DIR / f"{stem}.png"
    svg_path = OUT_DIR / f"{stem}.svg"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(svg_path, bbox_inches="tight")
    plt.close(fig)
    print(f"saved {png_path}")
    print(f"saved {svg_path}")


def style_axis(ax: plt.Axes, grid_axis: str = "y") -> None:
    ax.grid(axis=grid_axis, color=GRID, linewidth=0.8, alpha=0.75)
    ax.set_axisbelow(True)
    ax.tick_params(colors=TEXT)
    ax.xaxis.label.set_color(TEXT)
    ax.yaxis.label.set_color(TEXT)
    ax.title.set_color(TEXT)


def annotate_hbars(ax: plt.Axes, bars, fmt: str = "{:.2f}", pad: float = 0.012) -> None:
    x_min, x_max = ax.get_xlim()
    span = x_max - x_min
    for bar in bars:
        width = bar.get_width()
        y = bar.get_y() + bar.get_height() / 2
        ax.text(width + span * pad, y, fmt.format(width), va="center", ha="left", fontsize=8.8, color=TEXT)


def annotate_line_points(ax: plt.Axes, xs, ys, fmt_func, dy: float = 5, color: str = TEXT) -> None:
    for x, y in zip(xs, ys):
        ax.annotate(
            fmt_func(y),
            xy=(x, y),
            xytext=(0, dy),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8.3,
            color=color,
        )


# -----------------------------------------------------------------------------
# Benchmark 1: Neural vs TF-IDF Ranking Quality
# -----------------------------------------------------------------------------
def plot_benchmark_1() -> None:
    df = read_csv("bench1_neural_vs_tfidf_quality.csv")
    df = df[df["query"] != "AVERAGE"].copy()
    df["delta"] = df["neural_ndcg10"] - df["tfidf_ndcg10"]

    # Put the first query at the top in the horizontal chart.
    df = df.iloc[::-1].reset_index(drop=True)

    y = np.arange(len(df))
    height = 0.36

    fig, ax = plt.subplots(figsize=(11.5, 6.2))
    neural_bars = ax.barh(
        y + height / 2,
        df["neural_ndcg10"],
        height=height,
        color=NEURAL_BLUE,
        label="Neural model",
    )
    tfidf_bars = ax.barh(
        y - height / 2,
        df["tfidf_ndcg10"],
        height=height,
        color=TFIDF_ORANGE,
        label="TF-IDF",
    )

    ax.set_yticks(y)
    ax.set_yticklabels(df["query"])
    ax.set_xlabel("NDCG@10")
    ax.set_ylabel("Query")
    ax.set_title("Neural vs TF-IDF Ranking Quality")
    ax.set_xlim(0, 1.24)
    ax.legend(loc="lower right", frameon=True)
    style_axis(ax, grid_axis="x")

    annotate_hbars(ax, neural_bars, "{:.2f}", pad=0.006)
    annotate_hbars(ax, tfidf_bars, "{:.2f}", pad=0.006)

    # Delta column on the right side.
    delta_x = 1.12
    ax.text(delta_x, len(df) - 0.15, "Δ Neural - TF-IDF", ha="left", va="bottom", fontsize=9.2, color=MUTED)
    for yi, delta in zip(y, df["delta"]):
        sign = "+" if delta >= 0 else ""
        ax.text(delta_x, yi, f"{sign}{delta:.2f}", ha="left", va="center", fontsize=9, color=TEXT)

    save_figure(fig, "chart1_neural_vs_tfidf_quality")


# -----------------------------------------------------------------------------
# Benchmark 2: Neural vs TF-IDF Reranking Latency
# -----------------------------------------------------------------------------
def plot_benchmark_2() -> None:
    df = read_csv("bench2_neural_vs_tfidf_latency.csv")

    fig, ax = plt.subplots(figsize=(8.4, 4.9))
    ax.plot(df["n_articles"], df["neural_ms"], marker="o", linewidth=2.0, color=NEURAL_BLUE, label="Neural model")
    ax.plot(df["n_articles"], df["tfidf_ms"], marker="s", linewidth=2.0, color=TFIDF_ORANGE, label="TF-IDF")

    annotate_line_points(ax, df["n_articles"], df["neural_ms"], lambda v: f"{v:.0f} ms", dy=7, color=NEURAL_BLUE)
    annotate_line_points(ax, df["n_articles"], df["tfidf_ms"], lambda v: f"{v:.0f} ms", dy=-13, color=TFIDF_ORANGE)

    ax.set_xlabel("Candidate articles")
    ax.set_ylabel("Latency (ms)")
    ax.set_title("Reranking Latency")
    ax.set_xticks(df["n_articles"])
    ax.legend(loc="upper left", frameon=True)
    style_axis(ax, grid_axis="y")

    ax.annotate(
        "Neural reranking has moderately higher latency,\nbut provides stronger ranking quality.",
        xy=(0.52, 0.18),
        xycoords="axes fraction",
        ha="left",
        va="center",
        fontsize=9,
        color=MUTED,
        bbox=dict(boxstyle="round,pad=0.35", facecolor="white", edgecolor="#CCCCCC", alpha=0.95),
    )

    save_figure(fig, "chart2_neural_vs_tfidf_latency")


# -----------------------------------------------------------------------------
# Benchmark 3: Word vs BPE Tokenization Speed
# -----------------------------------------------------------------------------
def plot_benchmark_3() -> None:
    df = read_csv("bench3_word_vs_bpe_tokenization.csv")

    fig, ax = plt.subplots(figsize=(9.2, 5.1))
    ax.plot(df["chars"], df["word_us"], marker="o", linewidth=2.0, color=NEURAL_BLUE, label="Word tokenizer")
    ax.plot(df["chars"], df["bpe_us"], marker="s", linewidth=2.0, color=TFIDF_ORANGE, label="BPE tokenizer")

    # Log scales keep both tokenizers visible and prevent early x-ticks from overlapping.
    ax.set_xscale("log")
    ax.set_yscale("log")

    for x, y in zip(df["chars"], df["word_us"]):
        ax.annotate(
            f"{y:.1f}",
            xy=(x, y),
            xytext=(0, 12),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8.0,
            color=NEURAL_BLUE,
            bbox=dict(boxstyle="round,pad=0.18", facecolor="white", edgecolor="none", alpha=0.82),
        )

    for x, y in zip(df["chars"], df["bpe_us"]):
        ax.annotate(
            f"{y:.0f}",
            xy=(x, y),
            xytext=(0, 8),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8.0,
            color=TFIDF_ORANGE,
            bbox=dict(boxstyle="round,pad=0.18", facecolor="white", edgecolor="none", alpha=0.82),
        )

    ax.set_xlabel("Input length (characters)")
    ax.set_ylabel("Encoding time (microseconds, log scale)")
    ax.set_title("Word vs BPE Tokenization Speed")
    ax.set_xticks(df["chars"])
    ax.set_xticklabels([str(int(v)) for v in df["chars"]], rotation=0)
    ax.margins(x=0.08, y=0.2)
    ax.legend(loc="upper left", frameon=True)
    style_axis(ax, grid_axis="y")

    save_figure(fig, "chart3_word_vs_bpe_tokenization")


# -----------------------------------------------------------------------------
# Benchmark 4: OOV Robustness by Domain
# -----------------------------------------------------------------------------
def plot_benchmark_4() -> None:
    df = read_csv("bench4_tokenizer_oov_coverage.csv")

    fig, ax = plt.subplots(figsize=(8.2, 4.8))
    bars = ax.bar(df["domain"], df["word_oov_percent"], color=NEURAL_BLUE, width=0.58)

    ax.set_xlabel("Domain")
    ax.set_ylabel("Word OOV (%)")
    ax.set_title("Word-Level OOV by Domain")
    ax.set_ylim(0, 112)
    style_axis(ax, grid_axis="y")

    for bar in bars:
        h = bar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2, h + 2.5, f"{h:.0f}%", ha="center", va="bottom", fontsize=9, color=TEXT)

    ax.annotate(
        "BPE maintains 100% character coverage\nacross all tested domains.",
        xy=(0.98, 0.82),
        xycoords="axes fraction",
        ha="right",
        va="center",
        fontsize=9,
        color=MUTED,
        bbox=dict(boxstyle="round,pad=0.35", facecolor="white", edgecolor="#CCCCCC", alpha=0.95),
    )

    save_figure(fig, "chart4_tokenizer_oov_coverage")


# -----------------------------------------------------------------------------
# Benchmark 5a: Epoch Time vs Threads
# -----------------------------------------------------------------------------
def plot_benchmark_5a() -> None:
    df = read_csv("bench5_multithread_training.csv")
    best_idx = df["train_epoch_ms"].idxmin()
    best_threads = int(df.loc[best_idx, "threads"])
    best_time = float(df.loc[best_idx, "train_epoch_ms"])

    fig, ax = plt.subplots(figsize=(7.8, 4.7))
    ax.plot(df["threads"], df["train_epoch_ms"], marker="o", linewidth=2.0, color=NEURAL_BLUE)

    annotate_line_points(ax, df["threads"], df["train_epoch_ms"], lambda v: f"{v:.0f} ms", dy=7, color=NEURAL_BLUE)

    ax.scatter([best_threads], [best_time], s=90, color=TFIDF_ORANGE, zorder=4, label="Best result")
    ax.annotate(
        f"Optimal: {best_threads} threads",
        xy=(best_threads, best_time),
        xytext=(18, 24),
        textcoords="offset points",
        arrowprops=dict(arrowstyle="->", color=MUTED, linewidth=1.0),
        fontsize=9,
        color=MUTED,
    )

    ax.set_xlabel("Threads")
    ax.set_ylabel("One epoch time (ms)")
    ax.set_title("Training Time vs Thread Count")
    ax.set_xticks(df["threads"])
    ax.legend(frameon=True)
    style_axis(ax, grid_axis="y")

    save_figure(fig, "chart5a_training_time_threads")


# -----------------------------------------------------------------------------
# Benchmark 5b: Speedup vs Threads
# -----------------------------------------------------------------------------
def plot_benchmark_5b() -> None:
    df = read_csv("bench5_multithread_training.csv")
    best_idx = df["speedup_vs_1_thread"].idxmax()
    best_threads = int(df.loc[best_idx, "threads"])
    best_speedup = float(df.loc[best_idx, "speedup_vs_1_thread"])

    fig, ax = plt.subplots(figsize=(7.8, 4.7))
    ax.plot(df["threads"], df["speedup_vs_1_thread"], marker="o", linewidth=2.0, color=NEURAL_BLUE)

    annotate_line_points(ax, df["threads"], df["speedup_vs_1_thread"], lambda v: f"{v:.2f}x", dy=7, color=NEURAL_BLUE)

    ax.scatter([best_threads], [best_speedup], s=90, color=TFIDF_ORANGE, zorder=4, label="Best result")
    ax.annotate(
        f"Optimal: {best_threads} threads",
        xy=(best_threads, best_speedup),
        xytext=(18, -28),
        textcoords="offset points",
        arrowprops=dict(arrowstyle="->", color=MUTED, linewidth=1.0),
        fontsize=9,
        color=MUTED,
    )

    ax.set_xlabel("Threads")
    ax.set_ylabel("Speedup vs 1 thread")
    ax.set_title("Training Speedup vs Thread Count")
    ax.set_xticks(df["threads"])
    ax.legend(frameon=True)
    style_axis(ax, grid_axis="y")

    save_figure(fig, "chart5b_training_speedup_threads")


# -----------------------------------------------------------------------------
# Benchmark 6: Training Loss Curve
# -----------------------------------------------------------------------------
def plot_benchmark_6() -> None:
    df = read_csv("bench6_training_loss_curve.csv")
    if "loss_percent" not in df.columns:
        df["loss_percent"] = df["loss"] / df["loss"].iloc[0] * 100.0

    fig, ax = plt.subplots(figsize=(8.6, 4.9))
    ax.plot(df["epoch"], df["loss_percent"], marker="o", linewidth=2.0, color=LOSS_BLUE)

    for idx, (x, y) in enumerate(zip(df["epoch"], df["loss_percent"])):
        dy = 7 if idx % 2 == 0 else -13
        va = "bottom" if dy > 0 else "top"
        ax.annotate(
            f"{y:.1f}%",
            xy=(x, y),
            xytext=(0, dy),
            textcoords="offset points",
            ha="center",
            va=va,
            fontsize=7.4,
            color=LOSS_BLUE,
            bbox=dict(boxstyle="round,pad=0.14", facecolor="white", edgecolor="none", alpha=0.78),
        )

    ax.set_xlabel("Epoch")
    ax.set_ylabel("Training loss (% of initial)")
    ax.set_title("Training Contrastive Loss by Epoch")
    ax.set_xticks(df["epoch"])
    ax.set_ylim(0, 105)
    style_axis(ax, grid_axis="y")

    ax.annotate(
        "Loss decreases consistently,\nindicating stable convergence.",
        xy=(0.54, 0.66),
        xycoords="axes fraction",
        ha="left",
        va="center",
        fontsize=9,
        color=MUTED,
        bbox=dict(boxstyle="round,pad=0.35", facecolor="white", edgecolor="#CCCCCC", alpha=0.95),
    )

    save_figure(fig, "chart6_training_loss_curve")


def main() -> None:
    plot_benchmark_1()
    plot_benchmark_2()
    plot_benchmark_3()
    plot_benchmark_4()
    plot_benchmark_5a()
    plot_benchmark_5b()
    plot_benchmark_6()
    print("Done.")


if __name__ == "__main__":
    main()
