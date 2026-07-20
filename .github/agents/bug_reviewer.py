"""Bug reviewer agent (agent 2 of 3).

Frequency: daily (03:00 UTC) over main.

What it does:
- Scans CUDA/C++ sources under nbody_2d/ looking for:
  * CUDA API calls (cudaMalloc/cudaMemcpy/cudaMemset/cudaFree) without
    CUDA_CHECK on the same or following lines   -> mechanical.
  * Kernel launches (<<<...>>>) without cudaGetLastError() nearby
                                                    -> mechanical.
  * std::chrono timing in files that launch kernels but never call
    cudaDeviceSynchronize()                         -> human judgement.
- Mechanical findings open an issue with a suggested patch.
- Human findings open an issue marked "Requiere intervencion humana".
- Never modifies code and never touches main directly.
- At most 5 automatic issues per week (rate limited via common.py).

Run manually (dry-run, no GitHub writes):
    python .github/agents/bug_reviewer.py
"""

from __future__ import annotations

import os
import re
from pathlib import Path

from common import (
    create_github_issue,
    gemini_enabled,
    get_gemini_response,
    open_issue_exists,
    rate_limit_ok,
)

TITLE_PREFIX = "[agent:bug]"
LABELS = ["agent", "bug"]
REPO_ROOT = Path(os.environ.get("GITHUB_WORKSPACE", os.getcwd()))
SCAN_ROOT = REPO_ROOT / "nbody_2d"

CUDA_API_CALLS = ("cudaMalloc", "cudaMemcpy", "cudaMemset", "cudaFree")
KERNEL_LAUNCH = re.compile(r"<<<\s*[^>]*>>>")
SOURCE_EXTENSIONS = (".cu", ".cuh", ".cpp", ".h")


def iter_sources() -> list[Path]:
    if not SCAN_ROOT.exists():
        return []
    return [
        p for p in SCAN_ROOT.rglob("*")
        if p.suffix in SOURCE_EXTENSIONS and p.is_file()
    ]


def check_cuda_check(path: Path, lines: list[str]) -> list[dict]:
    findings = []
    for i, line in enumerate(lines):
        for call in CUDA_API_CALLS:
            if call + "(" in line and "CUDA_CHECK" not in line:
                context = "\n".join(lines[i:i + 3])
                if "CUDA_CHECK" not in context:
                    findings.append({
                        "kind": "mechanical",
                        "title": (
                            f"{TITLE_PREFIX} {call} sin CUDA_CHECK "
                            f"-- {path.relative_to(REPO_ROOT)}"
                        ),
                        "body": (
                            f"En `{path.relative_to(REPO_ROOT)}` linea {i + 1} "
                            f"se llama a `{call}` sin verificar el error.\n\n"
                            f"```cpp\n{line.strip()}\n```\n\n"
                            f"**Parche sugerido (mecanico):**\n"
                            f"```cpp\nCUDA_CHECK({line.strip()});\n```"
                        ),
                    })
                    break
    return findings


def check_kernel_launch_errors(path: Path, lines: list[str]) -> list[dict]:
    findings = []
    for i, line in enumerate(lines):
        if KERNEL_LAUNCH.search(line):
            window = "\n".join(lines[i:i + 6])
            if "cudaGetLastError" not in window:
                findings.append({
                    "kind": "mechanical",
                    "title": (
                        f"{TITLE_PREFIX} Kernel sin cudaGetLastError "
                        f"-- {path.relative_to(REPO_ROOT)}"
                    ),
                    "body": (
                        f"En `{path.relative_to(REPO_ROOT)}` linea {i + 1} se "
                        f"lanza un kernel sin llamar a `cudaGetLastError()` "
                        f"despues.\n\n"
                        f"```cpp\n{line.strip()}\n```\n\n"
                        f"**Parche sugerido (mecanico):**\n"
                        f"```cpp\n{line.strip()}\n"
                        f"CUDA_CHECK(cudaGetLastError());\n```"
                    ),
                })
    return findings


def check_timing_sync(path: Path, content: str) -> list[dict]:
    if (
        "std::chrono" in content
        and KERNEL_LAUNCH.search(content)
        and "cudaDeviceSynchronize" not in content
    ):
        return [{
            "kind": "human",
            "title": (
                f"{TITLE_PREFIX} Medicion sin cudaDeviceSynchronize "
                f"-- {path.relative_to(REPO_ROOT)}"
            ),
            "body": (
                f"`{path.relative_to(REPO_ROOT)}` mide tiempo con "
                f"`std::chrono` y lanza kernels, pero nunca llama a "
                f"`cudaDeviceSynchronize()`.\n\n"
                f"Requiere intervencion humana: verificar que las mediciones "
                f"sincronicen el device (el enunciado exige "
                f"`cudaDeviceSynchronize()` antes y despues del tramo medido)."
            ),
        }]
    return []


def gemini_review(findings: list[dict]) -> list[dict]:
    """Optional: ask Gemini to double-check mechanical findings."""
    if not gemini_enabled() or not findings:
        return findings
    sample = "\n\n".join(
        f"- {f['title']}" for f in findings[:10]
    )
    prompt = f"""Eres revisor de codigo CUDA. Confirma cuales de estos hallazgos
son bugs reales (mecanicos) y cuales son falsos positivos. Responde solo con
una lista: una linea por hallazgo, "REAL: <titulo>" o "FALSO: <titulo>".

Hallazgos:
{sample}
"""
    text = get_gemini_response(prompt)
    if not text:
        return findings
    confirmed = []
    for f in findings:
        key = f["title"]
        for line in text.splitlines():
            if line.startswith("FALSO:") and key.split("--")[0].strip() in line:
                print(f"[bug-reviewer] Gemini descarta (falso positivo): {key}")
                break
        else:
            confirmed.append(f)
    return confirmed


def main() -> int:
    print("== Bug reviewer agent ==")
    if not rate_limit_ok(TITLE_PREFIX):
        return 0

    findings: list[dict] = []
    for path in iter_sources():
        content = path.read_text(encoding="utf-8", errors="replace")
        lines = content.splitlines()
        findings.extend(check_cuda_check(path, lines))
        findings.extend(check_kernel_launch_errors(path, lines))
        findings.extend(check_timing_sync(path, content))

    findings = gemini_review(findings)

    if not findings:
        print("Sin hallazgos en el codigo CUDA/C++.")
        return 0

    created = 0
    for f in findings:
        if created >= 5:
            print("[bug-reviewer] Limite de 5 issues por ejecucion alcanzado.")
            break
        if open_issue_exists(f["title"]):
            print(f"[bug-reviewer] Ya existe issue abierto: {f['title']}")
            continue
        body = f["body"] + (
            "\n\n---\n_Creado por el agente revisor de bugs. "
            "Nunca modifica main directamente._"
        )
        if create_github_issue(f["title"], body, LABELS):
            created += 1

    print(f"[bug-reviewer] Hallazgos: {len(findings)}, issues creados: {created}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
