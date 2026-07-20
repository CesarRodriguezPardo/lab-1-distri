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
