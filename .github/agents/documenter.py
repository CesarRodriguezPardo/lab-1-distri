"""Documenter agent (agent 1 of 3).

Frequency: weekly (Mondays 09:00 UTC) and on every merge to main.

What it does:
- Reviews README.md and CHANGELOG.md for missing or outdated documentation.
- Mechanical findings (empty repo URL, broken/empty links, missing changelog
  sections) -> opens a PR with the fix and labels agent:auto-fix.
- Findings that require technical judgement (explaining a kernel, design
  decisions) -> opens an issue marked "Requiere intervencion humana".
- Never merges and never touches code outside .md files.
- At most 5 automatic issues/PRs per week (rate limited via common.py).

Run manually (dry-run, no GitHub writes):
    python .github/agents/documenter.py
"""

from __future__ import annotations

import os
import re

from common import (
    create_branch,
    create_github_issue,
    create_or_update_file,
    create_pull_request,
    gemini_enabled,
    get_file_content,
    get_gemini_response,
    get_main_sha,
    open_issue_exists,
    rate_limit_ok,
    repo_full_name,
    require_gemini,
    slugify,
)

TITLE_PREFIX = "[agent:docs]"
ISSUE_LABELS = ["agent", "documentation"]
PR_LABELS = ["agent", "agent:auto-fix", "documentation"]
REPO_ROOT = os.environ.get("GITHUB_WORKSPACE", os.getcwd())


def read_file(path: str) -> str | None:
    full = os.path.join(REPO_ROOT, path)
    if not os.path.exists(full):
        return None
    with open(full, encoding="utf-8") as fh:
        return fh.read()


def _repo_url() -> str:
    name = repo_full_name()
    return f"https://github.com/{name}"


# --------------------------------------------------------------- findings --


def rule_based_findings() -> list[dict]:
    """Mechanical documentation checks that do not need an LLM.

    Each finding includes auto-fix data (path, commit_msg, build_new_content)
    for mechanical findings that can be repaired automatically.
    """
    findings: list[dict] = []

    readme = read_file("README.md")
    changelog = read_file("CHANGELOG.md")

    # --- README checks ---

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
        url_match = re.search(r"URL del Repositorio:\*\*\s*\[\s*\]", readme)
        if url_match:
            replacement = f"**URL del Repositorio:** [{_repo_url()}]({_repo_url()})"
            new_content = readme[:url_match.start()] + replacement + readme[url_match.end():]
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} URL del repositorio vacia en README.md",
                "body": (
                    "La seccion `URL del Repositorio` del README tiene el "
                    "placeholder vacio `[]`.\n\n"
                    "**Fix aplicado automaticamente** en este PR."
                ),
                "fix_path": "README.md",
                "fix_content": new_content,
                "fix_commit_msg": "docs(readme): fill repository URL",
                "pr_title": "docs(readme): fill repository URL (auto-fix)",
            })

        # Empty markdown links like [text]()
        if re.search(r"\[[^\]]+\]\(\s*\)", readme):
            new_content = re.sub(r"\[([^\]]+)\]\(\s*\)", r"`\1`", readme)
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} Enlaces vacios en README.md",
                "body": (
                    "Se encontraron enlaces Markdown sin destino:\n\n"
                    + "\n".join(f"- `{link}`" for link in empty_links)
                    + "\n\n**Fix aplicado automaticamente**: enlaces vacios "
                    "reemplazados por texto inline."
                ),
                "fix_path": "README.md",
                "fix_content": new_content,
                "fix_commit_msg": "docs(readme): remove empty markdown links",
                "pr_title": "docs(readme): remove empty markdown links (auto-fix)",
            })

    # --- CHANGELOG checks ---

    CHANGELOG_TEMPLATE = (
        "# Changelog\n\n"
        "All notable changes to this project will be documented in this "
        "file.\n\n"
        "The format is based on [Keep a Changelog]"
        "(https://keepachangelog.com/en/1.1.0/).\n\n"
        "## [2.0.0-lab2] - 2026-07-24\n\n"
        "### Added\n\n- CUDA kernels, memory management, agents, CI.\n\n"
        "### Changed\n\n- Extended NBodySystem/NBodySimulator for GPU.\n\n"
        "### Fixed\n\n- Floating-point tolerance for CPU vs GPU regression.\n"
    )

    if changelog is None:
        findings.append({
            "kind": "mechanical",
            "title": f"{TITLE_PREFIX} CHANGELOG.md no encontrado",
            "body": (
                "No existe `CHANGELOG.md` en la raiz.\n\n"
                "**Fix aplicado automaticamente**: creado con la version "
                "`2.0.0-lab2` y secciones Added, Changed y Fixed."
            ),
            "fix_path": "CHANGELOG.md",
            "fix_content": CHANGELOG_TEMPLATE,
            "fix_commit_msg": "docs(changelog): create CHANGELOG.md (auto-fix)",
            "pr_title": "docs(changelog): create CHANGELOG.md (auto-fix)",
        })
    else:
        missing_sections = [
            s for s in ("### Added", "### Changed", "### Fixed")
            if s not in changelog
        ]
        if "2.0.0-lab2" not in changelog:
            new_content = (
                "## [2.0.0-lab2] - 2026-07-24\n\n"
                "### Added\n\n- CUDA kernels, memory management, agents, CI.\n\n"
                "### Changed\n\n- Extended for GPU.\n\n"
                "### Fixed\n\n- Tolerances.\n"
            )
            # Append after the Keep a Changelog reference line or at the end
            if "keepachangelog" in changelog.lower():
                lower = changelog.lower()
                idx = lower.find("keepachangelog")
                sep = changelog.find("\n\n", idx)
                idx = (sep + 2) if sep != -1 else len(changelog)
            else:
                idx = changelog.find("##") if "##" in changelog else len(changelog)
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} CHANGELOG.md sin entrada para 2.0.0-lab2",
                "body": (
                    "`CHANGELOG.md` no contiene la version `2.0.0-lab2`.\n\n"
                    "**Fix aplicado automaticamente** en este PR."
                ),
                "fix_path": "CHANGELOG.md",
                "fix_content": new_content,
                "fix_commit_msg": "docs(changelog): add 2.0.0-lab2 section (auto-fix)",
                "pr_title": "docs(changelog): add 2.0.0-lab2 section (auto-fix)",
            })
        elif missing_sections:
            new_content = changelog + "\n".join(
                f"### {s.lstrip('#').strip()}\n\n" for s in missing_sections
            )
            findings.append({
                "kind": "mechanical",
                "title": f"{TITLE_PREFIX} CHANGELOG.md con secciones incompletas",
                "body": (
                    f"Faltan secciones: "
                    + ", ".join(s.strip("# ") for s in missing_sections)
                    + ".\n\n"
                    "**Fix aplicado automaticamente** en este PR."
                ),
                "fix_path": "CHANGELOG.md",
                "fix_content": new_content,
                "fix_commit_msg": "docs(changelog): add missing sections (auto-fix)",
                "pr_title": "docs(changelog): add missing sections (auto-fix)",
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
                "body": (
                    f"Hallazgo detectado por el agente documentador (Gemini).\n\n"
                    f"**Fix sugerido (mecanico):** {fix}"
                ),
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


# --------------------------------------------------- auto-fix PR workflow --


def _open_auto_fix_pr(finding: dict) -> bool:
    """Create a branch, commit the fix, and open a PR for a mechanical finding.

    Returns True if the PR was created, False otherwise.
    """
    if not all(k in finding for k in ("fix_path", "fix_content", "pr_title")):
        return False

    sha = get_main_sha()
    if sha is None:
        print("[documenter] No se pudo obtener SHA de main, creando issue en su lugar.")
        return False

    slug = slugify(finding["title"].replace(TITLE_PREFIX, "").strip())
    branch = f"fix/docs/{slug}"
    if len(branch) > 250:  # GitHub branch name limit
        branch = branch[:250]

    # Get current file SHA for updates (required by Contents API for updates)
    current_content, file_sha = get_file_content(finding["fix_path"], ref="main")
    if current_content is not None and file_sha is None:
        print(f"[documenter] No se pudo obtener SHA de {finding['fix_path']}")
        return False

    # branch already exists? skip (from a previous run that wasn't merged)
    if not create_branch(branch, sha):
        print("[documenter] Branch ya existe o fallo al crearla, creando issue.")
        return False

    if not create_or_update_file(
        path=finding["fix_path"],
        content=finding["fix_content"],
        message=finding.get("fix_commit_msg", "docs: auto-fix by documenter agent"),
        branch=branch,
        sha=file_sha,
    ):
        print("[documenter] Fallo al commitear el fix, creando issue.")
        return False

    pr_body = (
        f"{finding['body']}\n\n"
        f"---\n_Abierto por el agente documentador. "
        f"El merge requiere aprobacion humana._"
    )
    if create_pull_request(
        head_branch=branch,
        title=finding["pr_title"],
        body=pr_body,
        labels=PR_LABELS,
    ):
        return True

    print("[documenter] PR creation failed, falling back to issue.")
    return False


# --------------------------------------------------------------- main ----


def main() -> int:
    print("== Documenter agent ==")
    require_gemini()
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
            print("[documenter] Limite de 5 issues/PRs por ejecucion alcanzado.")
            break

        dedup_title = f.get("pr_title") or f["title"]
        if open_issue_exists(dedup_title):
            print(f"[documenter] Ya existe issue/PR abierto: {dedup_title}")
            continue

        # Mechanical + has fix data -> try auto-PR first
        if f["kind"] == "mechanical" and "fix_path" in f:
            if _open_auto_fix_pr(f):
                created += 1
                continue
            # Fallback: create issue with fix suggestion
        elif f["kind"] == "mechanical":
            # Mechanical but no auto-fix data (Gemini mechanical findings)
            pass  # fall through to issue creation

        body = f["body"] + (
            "\n\n---\n_Creado por el agente documentador. "
            "Maximo 5 issues por ejecucion por semana._"
        )
        if create_github_issue(f["title"], body, ISSUE_LABELS):
            created += 1

    print(f"[documenter] Hallazgos: {len(findings)}, issues/PRs creados: {created}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
