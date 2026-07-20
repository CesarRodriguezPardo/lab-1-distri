"""Documenter agent (agent 1 of 3).

Frequency: weekly (Mondays 09:00 UTC) and on every merge to main.

What it does:
- Reviews README.md and CHANGELOG.md for missing or outdated documentation.
- Mechanical findings (empty repo URL, broken/empty links, missing changelog
  sections) -> opens an issue with an obvious fix suggestion.
- Findings that require technical judgement (explaining a kernel, design
  decisions) -> opens an issue marked "Requiere intervencion humana".
- Never opens a PR with code changes and never touches main directly.
- At most 5 automatic issues per week (rate limited via common.py).

Run manually (dry-run, no GitHub writes):
    python .github/agents/documenter.py
"""

from __future__ import annotations

import os
import re

from common import (
    create_github_issue,
    gemini_enabled,
    get_gemini_response,
    open_issue_exists,
    rate_limit_ok,
)

TITLE_PREFIX = "[agent:docs]"
LABELS = ["agent", "documentation"]
REPO_ROOT = os.environ.get("GITHUB_WORKSPACE", os.getcwd())


def read_file(path: str) -> str | None:
    full = os.path.join(REPO_ROOT, path)
    if not os.path.exists(full):
        return None
    with open(full, encoding="utf-8") as fh:
        return fh.read()


def rule_based_findings() -> list[dict]:
    """Mechanical documentation checks that do not need an LLM."""
    findings: list[dict] = []

    readme = read_file("README.md")
    changelog = read_file("CHANGELOG.md")

    if readme is None:
        findings.append({
            "kind": "mechanical",
            "title": f"{TITLE_PREFIX} README.md no encontrado",
            "body": (
                "El agente documentador no encontro `README.md` en la raiz "
                "del repositorio.\n\n"
                "**Fix sugerido (mecanico):** crear `README.md` con la "
                "plantilla del proyecto (equipo, roles, compilacion, uso)."
            ),
        })
    else:
        # Empty repository URL placeholder, e.g. "**URL del Repositorio:** []"
        if re.search(r"URL del Repositorio:\*\*\s*\[\s*\]", readme):
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} URL del repositorio vacia en README.md",
                "body": (
                    "La seccion `URL del Repositorio` del README tiene el "
                    "placeholder vacio `[]`.\n\n"
                    "**Fix sugerido (mecanico):** reemplazar por la URL real "
                    "del repositorio."
                ),
            })
        # Empty markdown links like [text]()
        empty_links = re.findall(r"\[[^\]]+\]\(\s*\)", readme)
        if empty_links:
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} Enlaces vacios en README.md",
                "body": (
                    "Se encontraron enlaces Markdown sin destino:\n\n"
                    + "\n".join(f"- `{link}`" for link in empty_links)
                    + "\n\n**Fix sugerido (mecanico):** completar o eliminar "
                    "los enlaces."
                ),
            })

    if changelog is None:
        findings.append({
            "kind": "mechanical",
            "title": f"{TITLE_PREFIX} CHANGELOG.md no encontrado",
            "body": (
                "No existe `CHANGELOG.md` en la raiz.\n\n"
                "**Fix sugerido (mecanico):** crear `CHANGELOG.md` con "
                "formato Keep a Changelog y la version `2.0.0-lab2` con "
                "secciones Added, Changed y Fixed."
            ),
        })
    else:
        missing_sections = [
            s for s in ("### Added", "### Changed", "### Fixed")
            if s not in changelog
        ]
        if "2.0.0-lab2" not in changelog:
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} CHANGELOG.md sin entrada para 2.0.0-lab2",
                "body": (
                    "`CHANGELOG.md` no contiene una seccion para la version "
                    "`2.0.0-lab2`.\n\n"
                    "**Fix sugerido (mecanico):** agregar "
                    "`## [2.0.0-lab2] - <fecha>` siguiendo Keep a Changelog."
                ),
            })
        elif missing_sections:
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} CHANGELOG.md con secciones incompletas",
                "body": (
                    "La entrada `2.0.0-lab2` del CHANGELOG no contiene las "
                    "secciones minimas:\n\n"
                    + "\n".join(f"- `{s}`" for s in missing_sections)
                    + "\n\n**Fix sugerido (mecanico):** completar Added, "
                    "Changed y Fixed."
                ),
            })

    return findings


def gemini_findings(readme: str | None, changelog: str | None) -> list[dict]:
    """Deeper documentation review with Gemini (optional)."""
    if not gemini_enabled():
        return []

    prompt = f"""Eres un revisor de documentacion para un laboratorio universitario
de simulacion N-body en C++/CUDA (kernels, memoria host/device, tests CPU vs GPU,
agentes de IA en CI).

Analiza el README.md y CHANGELOG.md siguientes y responde SOLO con una lista
de hallazgos de documentacion faltante o desactualizada. Para cada hallazgo usa
exactamente este formato:

MECANICO: <titulo corto> | <fix sugerido en una linea>
HUMANO: <titulo corto> | <motivo por el que requiere juicio tecnico>

Si no hay hallazgos, responde: SIN HALLAZGOS.
No inventes problemas de codigo; solo documentacion.

=== README.md ===
{(readme or '(no existe)')[:6000]}

=== CHANGELOG.md ===
{(changelog or '(no existe)')[:3000]}
"""
    text = get_gemini_response(prompt)
    if not text or "SIN HALLAZGOS" in text:
        return []

    findings: list[dict] = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("MECANICO:"):
            parts = line[len("MECANICO:"):].split("|", 1)
            title = parts[0].strip()
            fix = parts[1].strip() if len(parts) > 1 else ""
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} {title}",
                "body": f"Hallazgo detectado por el agente documentador (Gemini).\n\n"
                        f"**Fix sugerido (mecanico):** {fix}",
            })
        elif line.startswith("HUMANO:"):
            parts = line[len("HUMANO:"):].split("|", 1)
            title = parts[0].strip()
            reason = parts[1].strip() if len(parts) > 1 else ""
            findings.append({
                "kind": "human",
                "title": f"{TITLE_PREFIX} {title}",
                "body": f"Requiere intervencion humana: {reason}",
            })
    return findings


def main() -> int:
    print("== Documenter agent ==")
    if not rate_limit_ok(TITLE_PREFIX):
        return 0

    findings = rule_based_findings()
    readme = read_file("README.md")
    changelog = read_file("CHANGELOG.md")
    findings.extend(gemini_findings(readme, changelog))

    if not findings:
        print("Documentacion al dia. Sin hallazgos.")
        return 0

    created = 0
    for f in findings:
        if created >= 5:
            print("[documenter] Limite de 5 issues por ejecucion alcanzado.")
            break
        if open_issue_exists(f["title"]):
            print(f"[documenter] Ya existe issue abierto: {f['title']}")
            continue
        body = f["body"] + (
            "\n\n---\n_Creado por el agente documentador. "
            "Maximo 5 issues automaticos por semana._"
        )
        if create_github_issue(f["title"], body, LABELS):
            created += 1

    print(f"[documenter] Hallazgos: {len(findings)}, issues creados: {created}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
