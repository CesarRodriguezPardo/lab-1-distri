#!/usr/bin/env python3
"""Genera la figura unificada ``performance_plots.png`` del Lab 2 (N-Body 2D, C++/CUDA).

La figura 2x3 contiene las 6 graficas obligatorias del enunciado:

  (a) Speedup GPU vs N (kernel-only, mejor blockDim por variante).
  (b) Kernel-only vs End-to-End (kernel basico, mejor config por N).
  (c) Estudio de blockDim.x (kernel-only, basico).
  (d) Fraccion serial estimada y limite de Amdahl.
  (e) Trayectorias de 8 cuerpos + conservacion de energia (con inset).
  (f) Kernel basico vs shared-memory (speedup kernel-only vs N).

Archivos de entrada (buscados en ``--data-dir``):

  * ``benchmark_results.dat`` : salida de ``Benchmark::saveGpuResultsToFile``.
    Formato de 9 columnas separadas por espacios con una linea de cabecera:
    ``N_Bodies Variant BlockDim MeasureType AvgTime(s) StdDev(s) Speedup
    CpuStdDev(s) SpeedupError``.
    Se soporta tambien el formato legacy de 7 columnas (sin ``CpuStdDev(s)``
    ni ``SpeedupError``); en ese caso el error del speedup se aproxima como
    ``S * (StdDev/AvgTime)`` (se asume sigma_cpu despreciable) y se informa
    por stdout.
    Si el archivo no existe o es un placeholder con la palabra "pending",
    las celdas (a)-(d) y (f) quedan como "Sin datos".
  * ``energy_cuda.dat`` (o fallbacks ``energy_parallel_for.dat`` /
    ``energy_serial.dat``): cabecera ``K_Cinetica U_Potencial E_Total`` y
    una fila por paso.
  * ``trajectories_cuda.dat`` (o fallbacks ``trajectories_parallel_for.dat``
    / ``trajectories_serial.dat``): cabecera ``id step X Y VX VY`` y una
    fila por cuerpo/paso.
  * ``cluster_run.log`` (opcional): stdout de la corrida en el cluster DIINF.
    Se parsean lineas ``Midiendo N=... | Variante=... | BlockDim=...`` y
    lineas ``[EXITO] ...`` / ``[ERROR] ...``; el resumen se imprime por
    stdout y se anota en el pie de la figura.

El script NUNCA falla por datos faltantes: las celdas sin datos muestran un
texto explicativo y el PNG siempre se genera (salida exit 0 salvo error
inesperado).

Uso:
    python3 nbody_2d/scripts/generate_plots.py \
        [--data-dir DIR] [--out RUTA_PNG] [--log RUTA_LOG]

Defaults:
    --data-dir  directorio padre del script (nbody_2d/)
    --out       <dir_del_script>/performance_plots.png
    --log       <data-dir>/cluster_run.log

Dependencias: solo stdlib + numpy + pandas + matplotlib (backend "Agg").
"""

from __future__ import annotations

import argparse
import re
import sys
import warnings
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # backend sin display (cluster / CI)

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

# ---------------------------------------------------------------------------
# Constantes
# ---------------------------------------------------------------------------

BENCH_FILE = "benchmark_results.dat"
ENERGY_CANDIDATES = ("energy_cuda.dat", "energy_parallel_for.dat", "energy_serial.dat")
TRAJ_CANDIDATES = ("trajectories_cuda.dat", "trajectories_parallel_for.dat", "trajectories_serial.dat")

TEXT_NO_DATA = "Sin datos\n(pendiente ejecución en clúster DIINF)"

VARIANT_LABELS = {0: "básico", 1: "shared-memory"}
MEASURE_TYPES = ("kernel-only", "end-to-end")
N_TRAJ_IDS = 8  # cantidad de cuerpos a graficar en la celda (e)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Define y parsea los argumentos de linea de comandos."""
    script_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description="Genera performance_plots.png con las 6 gráficas del Lab 2."
    )
    parser.add_argument(
        "--data-dir",
        default=str(script_dir.parent),
        help="Directorio con los .dat (default: padre del script, nbody_2d/).",
    )
    parser.add_argument(
        "--out",
        default=str(script_dir / "performance_plots.png"),
        help="Ruta del PNG de salida (default: <dir_del_script>/performance_plots.png).",
    )
    parser.add_argument(
        "--log",
        default=None,
        help="Ruta del cluster_run.log (default: <data-dir>/cluster_run.log).",
    )
    return parser.parse_args(argv)


# ---------------------------------------------------------------------------
# Carga de datos
# ---------------------------------------------------------------------------


def _es_placeholder(texto: str) -> bool:
    """Detecta archivos placeholder que contienen la palabra 'pending'."""
    return "pending" in texto.lower()


def load_benchmark(path: Path) -> pd.DataFrame | None:
    """Carga ``benchmark_results.dat`` y normaliza los nombres de columna.

    Devuelve ``None`` si el archivo no existe, es placeholder, esta vacio o
    no se puede parsear. Soporta el formato actual de 9 columnas y el legacy
    de 7 columnas (aproximando SpeedupError).
    """
    if not path.exists():
        print(f"[datos] {path.name}: no encontrado.")
        return None
    try:
        if _es_placeholder(path.read_text(errors="replace")):
            print(f"[datos] {path.name}: placeholder 'pending' -> sin datos.")
            return None
        df = pd.read_csv(path, sep=r"\s+", engine="python")
    except Exception as exc:  # archivo corrupto o con formato inesperado
        print(f"[datos] {path.name}: no se pudo parsear ({exc}).")
        return None
    if df.empty:
        print(f"[datos] {path.name}: sin filas de datos.")
        return None

    rename = {
        "N_Bodies": "n",
        "Variant": "variant",
        "BlockDim": "blockdim",
        "MeasureType": "measure",
        "AvgTime(s)": "avg",
        "StdDev(s)": "std",
        "Speedup": "speedup",
        "CpuStdDev(s)": "cpu_std",
        "SpeedupError": "serr",
    }
    df = df.rename(columns=rename)
    columnas_requeridas = {"n", "variant", "blockdim", "measure", "avg", "std", "speedup"}
    if not columnas_requeridas.issubset(df.columns):
        print(f"[datos] {path.name}: columnas inesperadas {list(df.columns)}.")
        return None

    if "serr" not in df.columns:
        # Formato legacy de 7 columnas: sin CpuStdDev ni SpeedupError.
        # Aproximacion: sigma_S ~ S * (sigma_Tgpu / T_gpu), asumiendo sigma_cpu
        # despreciable frente a sigma_gpu en la formula (4).
        print(
            f"[datos] {path.name}: formato legacy de 7 columnas detectado; "
            "SpeedupError aproximado como S*(StdDev/AvgTime) "
            "(se asume sigma_cpu despreciable)."
        )
        df["cpu_std"] = np.nan
        df["serr"] = df["speedup"] * (df["std"] / df["avg"])

    for col in ("n", "variant", "blockdim"):
        df[col] = df[col].astype(int)
    print(f"[datos] {path.name}: cargado ({len(df)} filas).")
    return df


def _load_tabular(data_dir: Path, candidatos: tuple[str, ...]) -> tuple[pd.DataFrame | None, str | None]:
    """Carga un .dat whitespace-delimitado siguiendo el orden de preferencia.

    Devuelve ``(dataframe, nombre_archivo_usado)`` o ``(None, None)``.
    """
    for nombre in candidatos:
        ruta = data_dir / nombre
        if not ruta.exists():
            continue
        try:
            df = pd.read_csv(ruta, sep=r"\s+", engine="python")
        except Exception as exc:
            print(f"[datos] {nombre}: no se pudo parsear ({exc}).")
            return None, None
        if df.empty:
            continue
        etiqueta = nombre if nombre == candidatos[0] else f"{nombre} (fallback)"
        print(f"[datos] {nombre}: cargado ({len(df)} filas).")
        return df, etiqueta
    print(f"[datos] {'/'.join(candidatos)}: no encontrados.")
    return None, None


def load_energy(data_dir: Path) -> tuple[pd.DataFrame | None, str | None]:
    """Carga el archivo de energia (K, U, E por paso)."""
    return _load_tabular(data_dir, ENERGY_CANDIDATES)


def load_trajectories(data_dir: Path) -> tuple[pd.DataFrame | None, str | None]:
    """Carga el archivo de trayectorias (id, step, X, Y, VX, VY)."""
    return _load_tabular(data_dir, TRAJ_CANDIDATES)


def parse_cluster_log(path: Path) -> str:
    """Resume el log del cluster: n de mediciones y resultado de validacion."""
    if not path.exists():
        resumen = "cluster_run.log: no encontrado (opcional)."
        print(f"[log] {resumen}")
        return resumen
    texto = path.read_text(errors="replace")
    mediciones = re.findall(
        r"Midiendo\s+N=(\d+)\s*\|\s*Variante=(\d+)\s*\|\s*BlockDim=(\d+)", texto
    )
    exitos = re.findall(r"\[EXITO\].*", texto)
    errores = re.findall(r"\[ERROR\].*", texto)
    partes = [
        f"cluster_run.log: {len(mediciones)} mediciones, "
        f"{len(exitos)} [EXITO], {len(errores)} [ERROR]"
    ]
    if exitos:
        partes.append(f"último EXITO: \"{exitos[-1].strip()}\"")
    if errores:
        partes.append(f"último ERROR: \"{errores[-1].strip()}\"")
    resumen = " | ".join(partes)
    print(f"[log] {resumen}")
    return resumen


# ---------------------------------------------------------------------------
# Utilidades de graficado
# ---------------------------------------------------------------------------


def cell_no_data(ax: plt.Axes) -> None:
    """Dibuja la celda de relleno cuando no hay datos disponibles."""
    ax.text(
        0.5,
        0.5,
        TEXT_NO_DATA,
        transform=ax.transAxes,
        ha="center",
        va="center",
        fontsize=13,
        color="gray",
    )
    ax.axis("off")


def _mejor_por_speedup(df: pd.DataFrame, variant: int, measure: str) -> pd.DataFrame:
    """Filtra variante/tipo y toma, por N, la config con mayor Speedup."""
    sub = df[(df["variant"] == variant) & (df["measure"] == measure)]
    if sub.empty:
        return sub
    best = sub.loc[sub.groupby("n")["speedup"].idxmax()]
    return best.sort_values("n")


def _mejor_por_tiempo(df: pd.DataFrame, variant: int, measure: str) -> pd.DataFrame:
    """Filtra variante/tipo y toma, por N, la config con menor AvgTime."""
    sub = df[(df["variant"] == variant) & (df["measure"] == measure)]
    if sub.empty:
        return sub
    best = sub.loc[sub.groupby("n")["avg"].idxmin()]
    return best.sort_values("n")


# ---------------------------------------------------------------------------
# Celdas de la figura (una funcion por celda)
# ---------------------------------------------------------------------------


def _plot_speedup_lines(ax: plt.Axes, df: pd.DataFrame | None) -> bool:
    """Dibuja Speedup vs N (kernel-only, mejor blockDim) para cada variante.

    Helper comun de las celdas (a) y (f). Devuelve True si grafico algo.
    """
    if df is None:
        return False
    grafico_algo = False
    for variant in sorted(df["variant"].unique()):
        best = _mejor_por_speedup(df, variant, "kernel-only")
        if best.empty:
            continue
        ax.errorbar(
            best["n"],
            best["speedup"],
            yerr=best["serr"],
            marker="o",
            capsize=4,
            lw=1.5,
            label=VARIANT_LABELS.get(variant, f"variante {variant}"),
        )
        grafico_algo = True
    if grafico_algo:
        ax.set_xlabel("N (número de cuerpos)")
        ax.set_ylabel("Speedup")
        ax.grid(True, alpha=0.3)
        ax.legend()
    return grafico_algo


def plot_celda_a(ax: plt.Axes, df: pd.DataFrame | None) -> bool:
    """(a) Speedup GPU vs N (kernel-only, mejor blockDim)."""
    ax.set_title("(a) Speedup GPU vs N (kernel-only, mejor blockDim)")
    if not _plot_speedup_lines(ax, df):
        cell_no_data(ax)
        return False
    return True


def plot_celda_b(ax: plt.Axes, df: pd.DataFrame | None) -> bool:
    """(b) Kernel-only vs End-to-End para el kernel basico (mejor config por N)."""
    ax.set_title("(b) Kernel-only vs End-to-End (kernel básico)")
    if df is None:
        cell_no_data(ax)
        return False
    ns = sorted(df[df["variant"] == 0]["n"].unique())
    if not ns:
        cell_no_data(ax)
        return False

    x = np.arange(len(ns))
    width = 0.38
    tiempos: list[float] = []
    grafico_algo = False
    for i, mt in enumerate(MEASURE_TYPES):
        best = _mejor_por_tiempo(df, 0, mt).set_index("n").reindex(ns)
        vals = best["avg"].to_numpy(dtype=float)
        errs = best["std"].to_numpy(dtype=float)
        mask = ~np.isnan(vals)
        if not mask.any():
            continue
        ax.bar(
            (x + (i - 0.5) * width)[mask],
            vals[mask],
            width,
            yerr=errs[mask],
            capsize=4,
            alpha=0.85,
            label=mt,
        )
        tiempos.extend(vals[mask].tolist())
        grafico_algo = True

    if not grafico_algo:
        cell_no_data(ax)
        return False
    ax.set_xticks(x)
    ax.set_xticklabels([str(n) for n in ns])
    ax.set_xlabel("N (número de cuerpos)")
    ax.set_ylabel("Tiempo promedio (s)")
    # Escala logaritmica si el rango de tiempos supera dos ordenes de magnitud.
    if tiempos and min(tiempos) > 0 and max(tiempos) / min(tiempos) > 100:
        ax.set_yscale("log")
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    return True


def plot_celda_c(ax: plt.Axes, df: pd.DataFrame | None) -> bool:
    """(c) Estudio de blockDim.x: AvgTime vs BlockDim (log2) por cada N."""
    ax.set_title("(c) Estudio de blockDim.x (kernel-only, básico)")
    if df is None:
        cell_no_data(ax)
        return False
    sub = df[(df["variant"] == 0) & (df["measure"] == "kernel-only")]
    if sub.empty:
        cell_no_data(ax)
        return False

    for n in sorted(sub["n"].unique()):
        grupo = sub[sub["n"] == n].sort_values("blockdim")
        ax.errorbar(
            grupo["blockdim"],
            grupo["avg"],
            yerr=grupo["std"],
            marker="s",
            capsize=4,
            lw=1.5,
            label=f"N={n}",
        )
    ax.set_xscale("log", base=2)
    ax.set_xticks(sorted(sub["blockdim"].unique()))
    ax.set_xticklabels([str(b) for b in sorted(sub["blockdim"].unique())])
    ax.set_xlabel("blockDim.x (hilos por bloque)")
    ax.set_ylabel("Tiempo promedio kernel (s)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    return True


def plot_celda_d(ax: plt.Axes, df: pd.DataFrame | None) -> bool:
    """(d) Fraccion serial estimada f_s y limite de Amdahl (eje doble)."""
    ax.set_title("(d) Fracción serial estimada y límite de Amdahl")
    if df is None:
        cell_no_data(ax)
        return False
    kernel = _mejor_por_tiempo(df, 0, "kernel-only").set_index("n")
    e2e = _mejor_por_tiempo(df, 0, "end-to-end").set_index("n")
    ns = sorted(set(kernel.index) & set(e2e.index))
    if not ns:
        cell_no_data(ax)
        return False

    t_kernel = kernel.loc[ns, "avg"].to_numpy(dtype=float)
    t_e2e = e2e.loc[ns, "avg"].to_numpy(dtype=float)
    s_e2e = e2e.loc[ns, "speedup"].to_numpy(dtype=float)
    serr_e2e = e2e.loc[ns, "serr"].to_numpy(dtype=float)
    with np.errstate(divide="ignore", invalid="ignore"):
        f_s = np.clip((t_e2e - t_kernel) / t_e2e, 0.0, 1.0)

    # Eje izquierdo: fraccion serial estimada.
    ax.plot(ns, f_s, marker="o", color="tab:purple", lw=1.5, label="f_s estimada")
    ax.set_xlabel("N (número de cuerpos)")
    ax.set_ylabel("Fracción serial f_s", color="tab:purple")
    ax.tick_params(axis="y", labelcolor="tab:purple")
    ax.set_ylim(0, 1)
    ax.grid(True, alpha=0.3)

    # Eje derecho: speedup end-to-end medido y asintota de Amdahl 1/f_s.
    ax2 = ax.twinx()
    ax2.errorbar(
        ns,
        s_e2e,
        yerr=serr_e2e,
        marker="s",
        color="tab:orange",
        lw=1.5,
        capsize=4,
        label="Speedup end-to-end",
    )
    f_media = float(np.mean(f_s))
    if f_media > 0:
        asintota = 1.0 / f_media
        ax2.axhline(
            asintota,
            ls="--",
            color="tab:red",
            label=f"asíntota Amdahl 1/f_s ≈ {asintota:.1f}",
        )
    ax2.set_ylabel("Speedup end-to-end", color="tab:orange")
    ax2.tick_params(axis="y", labelcolor="tab:orange")

    # Leyenda combinada de ambos ejes (fondo opaco para legibilidad).
    lineas1, labels1 = ax.get_legend_handles_labels()
    lineas2, labels2 = ax2.get_legend_handles_labels()
    ax.legend(
        lineas1 + lineas2,
        labels1 + labels2,
        loc="best",
        fontsize=9,
        framealpha=1.0,
        edgecolor="gray",
    )
    return True


def plot_celda_e(
    ax: plt.Axes,
    traj: pd.DataFrame | None,
    traj_nombre: str | None,
    energy: pd.DataFrame | None,
    energy_nombre: str | None,
) -> bool:
    """(e) Trayectorias XY de 8 cuerpos + inset con K(t), U(t), E(t)."""
    grafico_algo = False

    if traj is not None and not traj.empty:
        ids = sorted(traj["id"].unique())[:N_TRAJ_IDS]
        for pid in ids:
            sub = traj[traj["id"] == pid].sort_values("step")
            (linea,) = ax.plot(sub["X"], sub["Y"], lw=1.0, label=f"id {pid}")
            # Marcador en el punto inicial de la trayectoria.
            ax.plot(
                sub["X"].iloc[0],
                sub["Y"].iloc[0],
                marker="o",
                ms=5,
                color=linea.get_color(),
            )
        ax.set_xlabel("X")
        ax.set_ylabel("Y")
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=7, loc="upper left")
        grafico_algo = True

    if energy is not None and not energy.empty:
        axins = inset_axes(ax, width="45%", height="42%", loc="lower right", borderpad=1.5)
        t = np.arange(len(energy))
        axins.plot(t, energy["K_Cinetica"], lw=1.0, label="K cinética")
        axins.plot(t, energy["U_Potencial"], lw=1.0, label="U potencial")
        axins.plot(t, energy["E_Total"], lw=1.0, label="E total")
        axins.set_title("Energía por paso", fontsize=8)
        axins.tick_params(labelsize=7)
        axins.grid(True, alpha=0.3)
        axins.legend(fontsize=6)
        grafico_algo = True

    if not grafico_algo:
        ax.set_title("(e) Trayectorias (8 cuerpos) + conservación de energía")
        cell_no_data(ax)
        return False

    # Si solo falta uno de los dos archivos, se anota dentro de la celda.
    if traj is None or traj.empty:
        ax.text(
            0.5,
            0.55,
            "Sin trayectorias",
            transform=ax.transAxes,
            ha="center",
            fontsize=10,
            color="gray",
        )

    ax.set_title(
        "(e) Trayectorias (8 cuerpos) + conservación de energía\n"
        f"[{traj_nombre or 'sin trayectorias'} | {energy_nombre or 'sin energía'}]",
        fontsize=11,
    )
    return True


def plot_celda_f(ax: plt.Axes, df: pd.DataFrame | None) -> bool:
    """(f) Kernel basico vs shared-memory: Speedup kernel-only vs N."""
    ax.set_title("(f) Kernel básico vs shared-memory")
    if not _plot_speedup_lines(ax, df):
        cell_no_data(ax)
        return False
    return True


# ---------------------------------------------------------------------------
# Programa principal
# ---------------------------------------------------------------------------


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    data_dir = Path(args.data_dir)
    out_path = Path(args.out)
    log_path = Path(args.log) if args.log else data_dir / "cluster_run.log"

    print("=== generate_plots.py — carga de datos ===")
    bench = load_benchmark(data_dir / BENCH_FILE)
    energy, energy_nombre = load_energy(data_dir)
    traj, traj_nombre = load_trajectories(data_dir)
    log_resumen = parse_cluster_log(log_path)

    fig, axes = plt.subplots(2, 3, figsize=(18, 10), dpi=150)
    fig.suptitle(
        "Simulador N-Body 2D — Análisis de Rendimiento GPU (Lab 2)", fontsize=16
    )

    # Cada celda se protege individualmente: un fallo inesperado en una celda
    # no impide generar el resto de la figura.
    celdas = [
        ("(a)", axes[0, 0], lambda ax: plot_celda_a(ax, bench)),
        ("(b)", axes[0, 1], lambda ax: plot_celda_b(ax, bench)),
        ("(c)", axes[0, 2], lambda ax: plot_celda_c(ax, bench)),
        ("(d)", axes[1, 0], lambda ax: plot_celda_d(ax, bench)),
        ("(e)", axes[1, 1], lambda ax: plot_celda_e(ax, traj, traj_nombre, energy, energy_nombre)),
        ("(f)", axes[1, 2], lambda ax: plot_celda_f(ax, bench)),
    ]
    sin_datos: list[str] = []
    for nombre, ax, dibujar in celdas:
        try:
            if not dibujar(ax):
                sin_datos.append(nombre)
        except Exception as exc:  # red de seguridad por celda
            print(f"[aviso] celda {nombre} falló inesperadamente: {exc}")
            cell_no_data(ax)
            sin_datos.append(nombre)

    # Pie de figura con el resumen del log del cluster.
    fig.text(0.5, 0.012, log_resumen, ha="center", va="bottom", fontsize=8, color="dimgray")
    with warnings.catch_warnings():
        # El inset_axes de la celda (e) no es compatible con tight_layout y
        # matplotlib emite un UserWarning inofensivo: se suprime para no
        # ensuciar el stdout/stderr del resumen.
        warnings.simplefilter("ignore", UserWarning)
        fig.tight_layout(rect=(0.0, 0.04, 1.0, 0.94))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path)
    plt.close(fig)

    print("=== Resumen ===")
    if sin_datos:
        print(f"Celdas sin datos: {', '.join(sin_datos)}")
    else:
        print("Celdas sin datos: ninguna.")
    print(log_resumen)
    print(f"PNG generado en: {out_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
