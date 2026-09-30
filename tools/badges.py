#!/usr/bin/env python3
"""Regenerate the README's progress badges from report.json.

The badge URLs are the only place this fork publishes its numbers, and they
were hand-written, so they drifted: before this tool they read 7250 functions
where report.json said 7251, and the percentages were truncated rather than
rounded. Anything hand-maintained and six-fields-wide drifts.

The README is also the snapshot. `--check` compares what the badges say against
what report.json says and exits non-zero if they differ, which is what a pull
request wants; `--write` rewrites them, which is what a push to the branch
wants. Deltas in the summary are computed against the values already in the
README, so there is no second build and no extra state file to keep in sync.

Usage:
  badges.py [--check | --write] [--summary] [--report PATH] [--readme PATH]

  --check    exit 1 if the README badges are out of date (prints a diff)
  --write    rewrite them in place
  --summary  print a GitHub-flavoured markdown table, with deltas, to stdout
"""

import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# label -> (badge text prefix, colour). Order is the order they appear.
BADGES = [
    ("Game Code", "Game%20Code", "limegreen"),
    ("Game Fuzzy", "Game%20Close%20Match", "yellowgreen"),
    ("Game Functions", "Game%20Functions", "lavender"),
    ("Overall", "Overall", "darkgreen"),
    ("Overall Fuzzy", "Overall%20Close%20Match", "olive"),
    ("Overall Functions", "Overall%20Functions", "slateblue"),
]

LINE = re.compile(r"^\[([^\]]+)\]: https://img\.shields\.io/badge/(\S+)$", re.M)


def measures(report):
    game = next(c["measures"] for c in report["categories"]
                if c["name"] == "Game Code")
    over = report["measures"]
    return {
        "Game Code": "%.2f%%" % game["matched_code_percent"],
        "Game Fuzzy": "%.2f%%" % game["fuzzy_match_percent"],
        "Game Functions": "%d / %d" % (game["matched_functions"],
                                       game["total_functions"]),
        "Overall": "%.2f%%" % over["matched_code_percent"],
        "Overall Fuzzy": "%.2f%%" % over["fuzzy_match_percent"],
        "Overall Functions": "%d / %d" % (over["matched_functions"],
                                          over["total_functions"]),
    }


def url(text, value, colour):
    # shields.io: a literal '-' in a field is '--'. Percent-encode LAST to
    # first, or the '%' of an already-inserted %20 becomes %2520.
    v = (value.replace("-", "--")
              .replace("%", "%25")
              .replace(" ", "%20")
              .replace("/", "%2F"))
    return "https://img.shields.io/badge/%s-%s-%s" % (text, v, colour)


def current(readme):
    """What the README's badges say right now, by label."""
    out = {}
    for label, body in LINE.findall(readme):
        m = re.match(r"^(.*?)-([^-].*?)-([a-z]+)$", body)
        if m:
            out[label] = (m.group(2).replace("%2F", "/")
                                    .replace("%20", " ")
                                    .replace("%25", "%"))
    return out


def main():
    rp = os.path.join(ROOT, "build/GQPE78/report.json")
    rm = os.path.join(ROOT, "README.md")
    if "--report" in sys.argv:
        rp = sys.argv[sys.argv.index("--report") + 1]
    if "--readme" in sys.argv:
        rm = sys.argv[sys.argv.index("--readme") + 1]

    want = measures(json.load(open(rp)))
    readme = open(rm, "rb").read().decode()
    have = current(readme)

    out = readme
    for label, text, colour in BADGES:
        out = re.sub(r"^\[%s\]: https://img\.shields\.io/badge/\S+$" % re.escape(label),
                     "[%s]: %s" % (label, url(text, want[label], colour)),
                     out, flags=re.M)

    if "--summary" in sys.argv:
        print("| metric | value | change |")
        print("|---|---|---|")
        for label, _, _ in BADGES:
            old, new = have.get(label), want[label]
            if old is None:
                delta = "new"
            elif old == new:
                delta = "&mdash;"
            elif new.endswith("%"):
                delta = "%+.2f" % (float(new[:-1]) - float(old[:-1]))
            else:
                delta = "%+d" % (int(new.split(" / ")[0]) - int(old.split(" / ")[0]))
            print("| %s | %s | %s |" % (label, new, delta))

    if "--write" in sys.argv:
        if out != readme:
            open(rm, "wb").write(out.encode())
            print("badges updated")
        else:
            print("badges already current")
        return 0

    if "--check" in sys.argv:
        if out != readme:
            print("README badges are out of date:")
            for label, _, _ in BADGES:
                if have.get(label) != want[label]:
                    print("  %-18s README %-16s report.json %s"
                          % (label, have.get(label, "(missing)"), want[label]))
            print("\nRun: python tools/badges.py --write")
            return 1
        print("README badges are current")
        return 0

    # Plain listing only when no mode was asked for. --summary already
    # printed the table, and the workflow pipes that straight into the job
    # summary, where a second copy in a different format is just noise.
    if "--summary" not in sys.argv:
        for label, _, _ in BADGES:
            print("%-18s %s" % (label, want[label]))
    return 0


sys.exit(main())
