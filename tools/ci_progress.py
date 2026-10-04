#!/usr/bin/env python3
"""Publish a static progress page from the existing, deduplicated objdiff report."""

import argparse
import html
import json
import os
from pathlib import Path
import shutil
import subprocess


VERSIONS = {"GQPE78": "USA", "GQPP78": "Europe", "GU4Y78": "Germany"}
DEFAULT_VERSION = "GQPE78"


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True, encoding="utf-8").strip()


def percent(measures: dict, matched: str, total: str) -> float:
    denominator = int(measures.get(total, 0))
    return 100 * int(measures.get(matched, 0)) / denominator if denominator else 0.0


def metrics(measures: dict) -> dict:
    return {
        "code": percent(measures, "matched_code", "total_code"),
        "data": percent(measures, "matched_data", "total_data"),
        "linked": percent(measures, "complete_code", "total_code"),
        "fuzzy": float(measures.get("fuzzy_match_percent", 0)),
        "functions": int(measures.get("matched_functions", 0)),
        "total_functions": int(measures.get("total_functions", 0)),
    }


def render(
    report: dict, commit: dict, repository: str, run_url: str,
    version: str = DEFAULT_VERSION, root_page: bool = True,
) -> str:
    prefix = "" if root_page else "../"
    version_links = " &middot; ".join(
        f'<a href="{prefix}{key}/"'
        + (' aria-current="page"' if key == version else '')
        + f'>{label} ({key})</a>'
        for key, label in VERSIONS.items()
    )
    overall = metrics(report["measures"])
    categories = [("All code", report["measures"])] + [
        (category["name"], category["measures"])
        for category in report.get("categories", [])
    ]
    rows = []
    for name, measures in categories:
        values = metrics(measures)
        rows.append(
            f'<tr><th scope="row">{html.escape(name)}</th>'
            f'<td>{values["code"]:.2f}%</td><td>{values["data"]:.2f}%</td>'
            f'<td>{values["fuzzy"]:.2f}%</td><td>{values["linked"]:.2f}%</td>'
            f'<td>{values["functions"]:,} / {values["total_functions"]:,}</td></tr>'
        )
    repo_url = html.escape(f"https://github.com/{repository}", quote=True)
    commit_url = f'{repo_url}/commit/{html.escape(commit["id"], quote=True)}'
    return f'''<!doctype html>
<html lang="en">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>BFBB decompilation progress - {VERSIONS[version]}</title>
<style>
  :root {{ color-scheme: dark; font-family: system-ui, sans-serif; background: #091c29; color: #e1f0f4; }}
  body {{ max-width: 1080px; margin: auto; padding: 48px 24px; line-height: 1.6; }}
  a {{ color: #79decd; }} h1 {{ line-height: 1.15; font-size: clamp(2rem, 5vw, 3.5rem); margin: 12px 0; }}
  nav {{ margin: 20px 0; }} nav [aria-current="page"] {{ font-weight: bold; color: #e1f0f4; }}
  .eyebrow {{ color: #79decd; letter-spacing: .14em; font-size: .8rem; text-transform: uppercase; }}
  .subtle, footer {{ color: #a4bdc9; }}
  .cards {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px; margin: 36px 0; }}
  .card {{ background: #122e40; padding: 24px; border-radius: 12px; }}
  .card strong {{ display: block; font-size: 2.5rem; color: #79decd; }}
  .table-wrap {{ overflow-x: auto; }} table {{ width: 100%; border-collapse: collapse; white-space: nowrap; }}
  th, td {{ text-align: right; padding: 14px 12px; border-bottom: 1px solid #294453; }}
  th:first-child {{ text-align: left; }} thead {{ color: #a4bdc9; }}
  footer {{ margin-top: 36px; font-size: .9rem; }} code {{ overflow-wrap: anywhere; }}
</style>
<main>
  <div class="eyebrow">{html.escape(repository.split("/")[0])} / {version} / GameCube</div>
  <h1>Battle for Bikini Bottom</h1>
  <nav aria-label="Game version">{version_links}</nav>
  <p class="subtle">Decompilation progress from a clean CI build of <a href="{repo_url}">{html.escape(repository)}</a>.</p>
  <div class="cards">
    <div class="card">Matching code<strong>{overall['code']:.2f}%</strong></div>
    <div class="card">Matching data<strong>{overall['data']:.2f}%</strong></div>
    <div class="card">Close match<strong>{overall['fuzzy']:.2f}%</strong></div>
  </div>
  <div class="table-wrap"><table>
    <thead><tr><th scope="col">Category</th><th scope="col">Code match</th><th scope="col">Data match</th>
    <th scope="col">Close match</th><th scope="col">Code linked</th><th scope="col">Functions matched</th></tr></thead>
    <tbody>{''.join(rows)}</tbody>
  </table></div>
  <p class="subtle">Code and data match measure exact matches against the original objects.
  Close match is objdiff's similarity score. Code linked measures source marked complete and linked into the verified executable.</p>
  <p><a href="report.json">Objdiff report</a> · <a href="progress.json">Progress JSON</a> ·
  <a href="api.json">Badge API</a> · <a href="{html.escape(run_url, quote=True)}">CI build</a></p>
  <footer>Commit <a href="{commit_url}"><code>{html.escape(commit['id'][:12])}</code></a> ·
  {html.escape(commit['time'])}<br>{html.escape(commit['message'])}</footer>
</main>
</html>
'''


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", choices=VERSIONS, default=DEFAULT_VERSION)
    parser.add_argument(
        "--version-page", action="store_true",
        help="render navigation for a page under its version directory",
    )
    args = parser.parse_args()
    report = json.loads((args.build_dir / "report.json").read_text(encoding="utf-8"))
    commit = {
        "id": git("rev-parse", "HEAD"),
        "time": git("show", "-s", "--format=%cI", "HEAD"),
        "message": git("show", "-s", "--format=%s", "HEAD"),
    }
    repository = os.environ.get("GITHUB_REPOSITORY", "zcanann/bfbb")
    run_id = os.environ.get("GITHUB_RUN_ID")
    run_url = f"https://github.com/{repository}/actions"
    if run_id:
        run_url += f"/runs/{run_id}"
    overall = metrics(report["measures"])
    api = {
        "version": args.version,
        "perfect_match": f"{overall['code']:.2f}%",
        "fuzzy_match": f"{overall['fuzzy']:.2f}%",
        "functions_matched": f"{overall['functions']:,} / {overall['total_functions']:,}",
        "commit": commit,
        "run_url": run_url,
    }
    commit_json = json.dumps(commit, indent=2) + "\n"
    (args.build_dir / "progress-commit.json").write_text(commit_json, encoding="utf-8")
    args.output.mkdir(parents=True, exist_ok=True)
    page = render(report, commit, repository, run_url, args.version, not args.version_page)
    (args.output / "index.html").write_text(page, encoding="utf-8")
    (args.output / "api.json").write_text(json.dumps(api, indent=2) + "\n", encoding="utf-8")
    # Explicit allowlist: never publish the build directory or original game files.
    for name in ("report.json", "progress.json", "progress-commit.json"):
        shutil.copyfile(args.build_dir / name, args.output / name)


if __name__ == "__main__":
    main()
