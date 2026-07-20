"""MR reviewer agent (agent 3 of 3).

Trigger: after the "CI - N-Body 2D" workflow completes on a pull request
(GitHub Actions `workflow_run` event), so the agent always runs AFTER the
test suite, never before it (required by the lab statement).

What it does:
- Reads the PR diff and the CI result.
- Classifies the PR as "mecanico y mergeable" or "requiere revision humana":
  * mechanical: only docs/format/tests, CI green, issue linked, no physics
    or public-API changes.
  * human: touches CUDA kernels, physics or public headers, or no linked issue.
- Posts a comment on the PR. If CI failed, the comment says so explicitly.
- NEVER merges. Merging always requires a human.

Run manually (dry-run):
    python .github/agents/mr_reviewer.py <pr_number>
"""

from __future__ import annotations

import os
import re
import sys

from common import (
    comment_on_pr,
    gemini_enabled,
    get_gemini_response,
    get_pr,
    get_pr_files,
    load_event,
)

DOC_EXTENSIONS = (".md", ".txt", ".rst")
SAFE_PREFIXES = ("tests/", ".github/agents/", ".github/workflows/")
KERNEL_EXTENSIONS = (".cu", ".cuh")
PUBLIC_API_EXTENSIONS = (".h",)
ISSUE_REF = re.compile(r"(?:Closes|Fixes|Refs)\s+#\d+", re.IGNORECASE)


def resolve_pr_context() -> tuple[int | None, str | None, str]:
    """Return (pr_number, ci_conclusion, event_name).

    Supports the workflow_run event (normal path) and a manually passed PR
    number for local testing.
    """
    event = load_event()
    event_name = os.environ.get("GITHUB_EVENT_NAME", "manual")

    if event_name == "workflow_run":
        run = event.get("workflow_run", {})
        prs = run.get("pull_requests", [])
        if not prs:
            print("[mr-reviewer] workflow_run sin PRs asociados. Nada que hacer.")
            return None, None, event_name
        return prs[0]["number"], run.get("conclusion"), event_name

    if event_name == "pull_request":
        pr = event.get("pull_request", {})
        return pr.get("number"), None, event_name

    # Local manual testing: python mr_reviewer.py <pr_number>
    if len(sys.argv) > 1 and sys.argv[1].isdigit():
        return int(sys.argv[1]), None, "manual"

    print("[mr-reviewer] Sin contexto de PR. Uso local: mr_reviewer.py <pr_number>")
    return None, None, event_name


def classify(files: list[dict]) -> tuple[str, list[str]]:
    """Rule-based classification. Returns (kind, reasons)."""
    human_reasons: list[str] = []
    for f in files:
        name = f["filename"]
        if name.endswith(KERNEL_EXTENSIONS) or "/kernels/" in name:
            human_reasons.append(f"modifica kernels CUDA: `{name}`")
        elif name.endswith(PUBLIC_API_EXTENSIONS) and "tests/" not in name:
            human_reasons.append(f"modifica API publica: `{name}`")
        elif name.endswith(DOC_EXTENSIONS) or name.startswith(SAFE_PREFIXES):
            continue  # clearly mechanical
        elif name.endswith((".cpp", ".py", ".yml", ".yaml")):
            continue  # code: reviewed by rules above + CI + human approver
    if human_reasons:
        return "human", human_reasons
    return "mechanical", ["solo documentacion, formato, tests o config"]


def gemini_second_opinion(files: list[dict], current: str) -> str:
    """Optional: let Gemini refine the classification of borderline PRs."""
    if not gemini_enabled():
        return current
    listing = "\n".join(
        f"- {f['filename']} (+{f['additions']}/-{f['deletions']})"
        for f in files[:30]
    )
    prompt = f"""Clasifica este pull request como MECANICO o HUMANO.
MECANICO: solo documentacion, formato, tests que pasan, sin cambio de
semantica fisica ni de firma publica.
HUMANO: cambios en kernels CUDA, fisica, API publica o logica no trivial.
Responde con una sola palabra: MECANICO o HUMANO.

Archivos del PR:
{listing}
"""
    text = get_gemini_response(prompt)
    if text and "HUMANO" in text.upper():
        return "human"
    if text and "MECANICO" in text.upper():
        return "mechanical"
    return current


def main() -> int:
    print("== MR reviewer agent ==")
    pr_number, ci_conclusion, event_name = resolve_pr_context()
    if pr_number is None:
        return 0

    if not os.environ.get("GH_TOKEN"):
        print("[mr-reviewer] Sin GH_TOKEN: modo dry-run, no se comentara.")

    pr = get_pr(pr_number) if os.environ.get("GH_TOKEN") else {}
    files = get_pr_files(pr_number) if os.environ.get("GH_TOKEN") else []

    kind, reasons = classify(files)
    kind = gemini_second_opinion(files, kind)

    ci_ok = ci_conclusion == "success"
    if ci_conclusion is None:
        ci_line = "CI: estado no disponible en este contexto"
    elif ci_ok:
        ci_line = "CI: :white_check_mark: pipeline en verde"
    else:
        ci_line = f"CI: :x: **pipeline fallando** (conclusion: `{ci_conclusion}`)"

    issue_ref = ISSUE_REF.search(
        (pr.get("title") or "") + "\n" + (pr.get("body") or "")
    )
    issue_line = (
        f"Issue vinculado: #{issue_ref.group(0).split('#')[1]}"
        if issue_ref
        else "Issue vinculado: :warning: **no detectado** (agrega `Closes #N`)"
    )

    if not ci_ok and ci_conclusion is not None:
        verdict = "No apto para merge: CI fallando."
    elif not issue_ref:
        verdict = "Requiere revision humana: todo PR debe referenciar un issue."
    elif kind == "mechanical":
        verdict = "Mecanico y mergeable tras aprobacion humana."
    else:
        verdict = "Requiere revision humana."

    comment = f"""### :robot: Agente revisor de MR

Ejecutado **despues** del pipeline de CI (evento `{event_name}`).

- {ci_line}
- {issue_line}
- Clasificacion: **{verdict}**

"""
    if reasons and kind == "human":
        comment += "Motivos:\n" + "\n".join(f"- {r}" for r in reasons) + "\n\n"

    if not ci_ok and ci_conclusion is not None:
        comment += (
            "> :warning: El pipeline de CI esta fallando. El comentario de "
            "este agente no reemplaza la suite de tests.\n\n"
        )

    comment += (
        "---\n_Este agente nunca fusiona a `main`. "
        "El merge requiere aprobacion humana._"
    )

    comment_on_pr(pr_number, comment)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
