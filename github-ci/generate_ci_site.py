import re
import argparse
import json
from jinja2 import Environment, FileSystemLoader
from pathlib import Path
from datetime import datetime, UTC


def format_datetime(value):
    return datetime.fromisoformat(value)


def get_time_class(time_started):
    if time_started is not None and time_started != "":
        dtime_started = format_datetime(time_started)
        diff = datetime.now(UTC) - dtime_started
        if diff.total_seconds() < 3600 * 24:
            return "short"
        if diff.total_seconds() < 3600 * 24 * 7:
            return "medium"
        if diff.total_seconds() < 3600 * 24 * 30:
            return "long"
    return "verylong"


def format_time(time_started):
    if time_started is None or time_started == "":
        return "unknown"
    dtime_started = format_datetime(time_started)
    diff = datetime.now(UTC) - dtime_started
    if diff.total_seconds() < 3600 * 24:
        return "today"
    if diff.total_seconds() < 3600 * 24 * 7:
        return "this week"
    if diff.total_seconds() < 3600 * 24 * 30:
        return "this month"
    return "long ago"


def get_duration_class(time_started, time_ended):
    if time_started is not None and time_started != "" and time_ended is not None and time_ended != "":
        dtime_started = format_datetime(time_started)
        dtime_ended = format_datetime(time_ended)
        diff = dtime_ended - dtime_started
        if diff.total_seconds() < 300:
            return "short"
        if diff.total_seconds() < 600:
            return "medium"
        if diff.total_seconds() < 3600:
            return "long"
    return "verylong"


def get_branch_pr_count(branch_count, pr_count):
    if branch_count - pr_count <= 0:
        return "good"
    if branch_count - pr_count <= 1:
        return "bad"
    return "verybad"


def format_duration(time_started, time_ended):
    if time_started is not None and time_started != "" and time_ended is not None and time_ended != "":
        dtime_started = format_datetime(time_started)
        dtime_ended = format_datetime(time_ended)
        diff = dtime_ended - dtime_started
        if diff.total_seconds() < 60:
            return f"in {diff.total_seconds():.0f} s"
        if diff.total_seconds() < 3600:
            return f"in {diff.total_seconds() / 60.0:.2f} m"
        return f"in {diff.total_seconds() / 3600.0:.2f} h"
    return "unknown"


def generate_site(json_input_path):
    """Render html files from templates to generate the site."""
    with open(json_input_path, "r") as f:
        repos = json.load(f)

    env = Environment(loader=FileSystemLoader("templates"))
    env.filters["format_datetime"] = format_datetime
    env.filters["get_time_class"] = get_time_class
    env.filters["format_time"] = format_time
    env.filters["get_duration_class"] = get_duration_class
    env.filters["format_duration"] = format_duration
    env.filters["get_branch_pr_count"] = get_branch_pr_count

    index_template = env.get_template("index_template.html")

    total_issues = sum(repo["open_issues"] for repo in repos)
    total_prs = sum(repo["open_prs"] for repo in repos)

    ci_repos = []
    noci_repos = []
    for repo in repos:
        if repo.get("build_develop") != None:
            ci_repos.append(repo)
        else:
            noci_repos.append(repo)

    total_repos = len(ci_repos)
    passing_repos = sum(
        1
        for repo in ci_repos
        if repo.get("build_develop", {}).get("conclusion") == "success"
    )

    passing_percentage = (
        round((passing_repos / total_repos) * 100, 1) if total_repos else 0
    )

    last_updated = datetime.now(UTC).strftime("%Y-%m-%d %H:%M UTC")
    workflow_badges = [
        {
            "image": "https://github.com/Mu2e/.github/actions/workflows/mu2e-lcov.yml/badge.svg",
            "link": "https://github.com/Mu2e/.github/actions/workflows/mu2e-lcov.yml",
            "alt": "Create LCOV coverage report",
        },
        {
            "image": "https://github.com/Mu2e/daq-docker/actions/workflows/mu2e-spack-selfhosted.yaml/badge.svg",
            "link": "https://github.com/Mu2e/daq-docker/actions/workflows/mu2e-spack-selfhosted.yaml",
            "alt": "Build mu2e-spack docker image (self hosted)",
        },
    ]

    # Content of the index page
    context = {
        "ci_repos": ci_repos,
        "noci_repos": noci_repos,
        "last_updated": last_updated,
        "total_issues": total_issues,
        "total_prs": total_prs,
        "passing_percentage": passing_percentage,
        "workflow_badges": workflow_badges,
    }

    index_template = env.get_template("index_template.html")
    index_html = index_template.render(context)
    index_path = Path("site/index.html")
    index_path.parent.mkdir(parents=True, exist_ok=True)
    index_path.write_text(index_html)

    print("Done")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate CI HTML site from a JSON summary file."
    )
    parser.add_argument(
        "--json_input",
        required=True,
        help="Path to the JSON file containing CI summary data. See collect-ci-metrics.sh.",
    )
    args = parser.parse_args()

    generate_site(args.json_input)
