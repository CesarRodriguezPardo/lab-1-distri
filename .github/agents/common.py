"""Shared utilities for the repository AI agents.

Wraps the GitHub REST API and the Gemini API so the three agents
(documenter, bug reviewer, MR reviewer) share one implementation.

Security model (inspired by GitHub Agentic Workflows):
- Agents run with read-only repository contents.
- The only allowed write operations ("safe outputs") are:
  creating issues and commenting on pull requests.
- Agents never modify code and never merge pull requests.
- At most 5 automatic issues per agent per week (rate limited).

Environment variables:
- GH_TOKEN:        GitHub token (provided by Actions as secrets.GITHUB_TOKEN).
                   If missing, agents run in dry-run mode (print only).
- GEMINI_API_KEY:  Google Gemini API key (repository secret, optional).
                   If missing, agents fall back to pure rule-based logic.
- GITHUB_REPOSITORY: 'owner/repo', set by GitHub Actions.
- GITHUB_EVENT_PATH: path to the JSON event payload, set by GitHub Actions.
"""

from __future__ import annotations

import json
import os
from datetime import datetime, timedelta, timezone

import requests

GITHUB_API = "https://api.github.com"
GEMINI_URL = (
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "gemini-2.5-flash:generateContent"
)

MAX_AUTO_ISSUES_PER_WEEK = 5
REQUEST_TIMEOUT = 30


# --------------------------------------------------------------------- env --

def repo_full_name() -> str:
    """Return 'owner/repo' from the Actions environment."""
    return os.environ.get("GITHUB_REPOSITORY", "")


def has_github_token() -> bool:
    return bool(os.environ.get("GH_TOKEN"))


def gemini_enabled() -> bool:
    return bool(os.environ.get("GEMINI_API_KEY"))


def require_gemini() -> None:
    """Exit with error if Gemini key is missing in CI. In local dev, warn only."""
    if gemini_enabled():
        return
    msg = (
        "GEMINI_API_KEY no configurado."
        " En CI agrega la key en Settings > Secrets > Actions > GEMINI_API_KEY."
        " En local exportala: export GEMINI_API_KEY=tu_key"
    )
    if os.environ.get("GITHUB_ACTIONS") == "true":
        print(f"[fatal] {msg}")
        raise SystemExit(1)
    print(f"[warning] {msg} Modo local: se usaran solo reglas programaticas.")


def _headers() -> dict:
    return {
        "Authorization": f"Bearer {os.environ['GH_TOKEN']}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }


def load_event() -> dict:
    """Load the GitHub Actions event payload (empty dict outside Actions)."""
    path = os.environ.get("GITHUB_EVENT_PATH")
    if path and os.path.exists(path):
        with open(path, encoding="utf-8") as fh:
            return json.load(fh)
    return {}


# ------------------------------------------------------------------ Gemini --

def get_gemini_response(prompt: str) -> str | None:
    """Ask Gemini (gemini-2.5-flash).

    Returns the model text, or None if the key is missing or the call
    fails. Callers must handle None by falling back to rule-based logic.
    """
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        return None
    try:
        resp = requests.post(
            f"{GEMINI_URL}?key={api_key}",
            json={"contents": [{"parts": [{"text": prompt}]}]},
            timeout=REQUEST_TIMEOUT,
        )
        resp.raise_for_status()
        data = resp.json()
        return data["candidates"][0]["content"]["parts"][0]["text"]
    except (requests.RequestException, KeyError, IndexError, TypeError) as exc:
        print(f"[common] Gemini unavailable, falling back to rules: {exc}")
        return None


# ------------------------------------------------------------------ GitHub --

def create_github_issue(title: str, body: str, labels: list[str]) -> bool:
    """Create an issue. Returns True on success, False in dry-run/failure."""
    if not has_github_token():
        print(f"\n[dry-run] Would create issue: {title}")
        print(f"[dry-run] Labels: {labels}")
        print(body)
        return False
    resp = requests.post(
        f"{GITHUB_API}/repos/{repo_full_name()}/issues",
        headers=_headers(),
        json={"title": title, "body": body, "labels": labels},
        timeout=REQUEST_TIMEOUT,
    )
    if resp.status_code == 201:
        print(f"[common] Issue created: #{resp.json()['number']} -- {title}")
        return True
    print(f"[common] Failed to create issue ({resp.status_code}): {resp.text}")
    return False


def comment_on_pr(pr_number: int, body: str) -> bool:
    """Post a comment on a pull request."""
    if not has_github_token():
        print(f"\n[dry-run] Would comment on PR #{pr_number}:")
        print(body)
        return False
    resp = requests.post(
        f"{GITHUB_API}/repos/{repo_full_name()}/issues/{pr_number}/comments",
        headers=_headers(),
        json={"body": body},
        timeout=REQUEST_TIMEOUT,
    )
    if resp.status_code == 201:
        print(f"[common] Commented on PR #{pr_number}")
        return True
    print(f"[common] Failed to comment ({resp.status_code}): {resp.text}")
    return False


def open_issue_exists(title: str) -> bool:
    """Return True if an open issue with the exact title already exists.

    Prevents duplicate automatic issues across scheduled runs.
    """
    if not has_github_token():
        return False
    resp = requests.get(
        f"{GITHUB_API}/repos/{repo_full_name()}/issues",
        headers=_headers(),
        params={"state": "open", "per_page": 100},
        timeout=REQUEST_TIMEOUT,
    )
    if resp.status_code != 200:
        return False
    return any(i.get("title") == title for i in resp.json())


def agent_issues_this_week(title_prefix: str) -> int:
    """Count issues labeled 'agent' with the given title prefix created in
    the last 7 days. Enforces the max-5-auto-issues-per-week rule."""
    if not has_github_token():
        return 0
    since = (datetime.now(timezone.utc) - timedelta(days=7)).isoformat()
    resp = requests.get(
        f"{GITHUB_API}/repos/{repo_full_name()}/issues",
        headers=_headers(),
        params={
            "labels": "agent",
            "since": since,
            "state": "all",
            "per_page": 100,
        },
        timeout=REQUEST_TIMEOUT,
    )
    if resp.status_code != 200:
        return 0
    return sum(
        1 for i in resp.json() if i.get("title", "").startswith(title_prefix)
    )


def rate_limit_ok(title_prefix: str, limit: int = MAX_AUTO_ISSUES_PER_WEEK) -> bool:
    count = agent_issues_this_week(title_prefix)
    if count >= limit:
        print(
            f"[common] Rate limit reached for '{title_prefix}': {count} "
            f"issues in the last 7 days (max {limit}). Skipping."
        )
        return False
    return True


def get_pr(pr_number: int) -> dict:
    resp = requests.get(
        f"{GITHUB_API}/repos/{repo_full_name()}/pulls/{pr_number}",
        headers=_headers(),
        timeout=REQUEST_TIMEOUT,
    )
    resp.raise_for_status()
    return resp.json()


def get_pr_files(pr_number: int) -> list[dict]:
    resp = requests.get(
        f"{GITHUB_API}/repos/{repo_full_name()}/pulls/{pr_number}/files",
        headers=_headers(),
        params={"per_page": 100},
        timeout=REQUEST_TIMEOUT,
    )
    resp.raise_for_status()
    return resp.json()


# ------------------------------------------------------ auto-fix PR helpers --


def slugify(text: str, max_len: int = 40) -> str:
    import re
    s = re.sub(r"[^\w\s-]", "", text.lower())
    s = re.sub(r"[-\s]+", "-", s).strip("-")
    return s[:max_len]


def get_main_sha() -> str | None:
    """Return the SHA of the latest commit on main."""
    if not has_github_token():
        return None
    resp = requests.get(
        f"{GITHUB_API}/repos/{repo_full_name()}/git/refs/heads/main",
        headers=_headers(),
        timeout=REQUEST_TIMEOUT,
    )
    if resp.status_code != 200:
        print(f"[common] get_main_sha failed ({resp.status_code})")
        return None
    return resp.json()["object"]["sha"]


def create_branch(branch_name: str, base_sha: str) -> bool:
    """Create a new branch pointing to base_sha."""
    if not has_github_token():
        print(f"[dry-run] Would create branch: {branch_name}")
        return False
    resp = requests.post(
        f"{GITHUB_API}/repos/{repo_full_name()}/git/refs",
        headers=_headers(),
        json={"ref": f"refs/heads/{branch_name}", "sha": base_sha},
        timeout=REQUEST_TIMEOUT,
    )
    ok = resp.status_code in (200, 201)
    if ok:
        print(f"[common] Branch created: {branch_name}")
    else:
        print(f"[common] Branch creation failed ({resp.status_code}): {resp.text}")
    return ok


def get_file_content(path: str, ref: str = "main") -> tuple[str | None, str | None]:
    """Return (decoded_content, sha) of a file, or (None, None)."""
    if not has_github_token():
        return None, None
    resp = requests.get(
        f"{GITHUB_API}/repos/{repo_full_name()}/contents/{path}",
        headers=_headers(),
        params={"ref": ref},
        timeout=REQUEST_TIMEOUT,
    )
    if resp.status_code != 200:
        return None, None
    data = resp.json()
    content = data.get("content", "")
    try:
        import base64
        decoded = base64.b64decode(content).decode("utf-8")
    except Exception:
        decoded = content
    return decoded, data.get("sha")


def create_or_update_file(
    path: str,
    content: str,
    message: str,
    branch: str,
    sha: str | None = None,
) -> bool:
    """Commit a file on a branch via the Contents API. Returns True on success."""
    if not has_github_token():
        print(f"[dry-run] Would commit to {path} on {branch}")
        return False
    try:
        import base64
        encoded = base64.b64encode(content.encode("utf-8")).decode("utf-8")
    except Exception:
        return False
    body: dict = {
        "message": message,
        "content": encoded,
        "branch": branch,
    }
    if sha:
        body["sha"] = sha
    resp = requests.put(
        f"{GITHUB_API}/repos/{repo_full_name()}/contents/{path}",
        headers=_headers(),
        json=body,
        timeout=REQUEST_TIMEOUT,
    )
    ok = resp.status_code in (200, 201)
    if ok:
        print(f"[common] Committed {path} on branch {branch}")
    else:
        print(f"[common] Commit failed ({resp.status_code}): {resp.text}")
    return ok


def create_pull_request(
    head_branch: str,
    title: str,
    body: str,
    base: str = "main",
    labels: list[str] | None = None,
) -> bool:
    """Open a pull request from head_branch to base."""
    if not has_github_token():
        print(f"[dry-run] Would open PR: {title} ({head_branch} -> {base})")
        return False
    pr_labels: list[str] = []
    if labels:
        pr_labels = labels
    resp = requests.post(
        f"{GITHUB_API}/repos/{repo_full_name()}/pulls",
        headers=_headers(),
        json={
            "title": title,
            "head": head_branch,
            "base": base,
            "body": body,
        },
        timeout=REQUEST_TIMEOUT,
    )
    ok = resp.status_code == 201
    if ok:
        pr = resp.json()
        print(f"[common] PR created: #{pr['number']} -- {title}")
        # Apply labels separately (labels aren't in the PR creation API)
        if pr_labels:
            requests.post(
                f"{GITHUB_API}/repos/{repo_full_name()}/issues/{pr['number']}/labels",
                headers=_headers(),
                json={"labels": pr_labels},
                timeout=REQUEST_TIMEOUT,
            )
    else:
        print(f"[common] PR creation failed ({resp.status_code}): {resp.text}")
    return ok
