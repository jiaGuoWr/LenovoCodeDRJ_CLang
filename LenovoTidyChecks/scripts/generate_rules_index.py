#!/usr/bin/env python3
"""Keep LenovoTidyChecks rule coverage in sync with the C# AnalyzerRules.md.

Compares:
  * C# rule IDs parsed from ../AnalyzerRules.md (CHN###, SEC###, EXC###,
    CODE###, NAME###)
  * C++ rule ids registered in src/LenovoTidyModule/LenovoTidyModule.cpp
  * Rule pages in docs/rules/*.md

Exits non-zero when:
  * A C++ check is registered but has no docs page.
  * A docs page exists for a rule id that is not registered in the module.

The script is intentionally conservative: it does NOT fail when a C# rule is
not yet ported — porting status is tracked in CHANGELOG.md instead.
"""
from __future__ import annotations

import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
ANALYZER_RULES = REPO.parent / "AnalyzerRules.md"
MODULE_CPP = REPO / "src" / "LenovoTidyModule" / "LenovoTidyModule.cpp"
DOCS_RULES = REPO / "docs" / "rules"

RULE_ID_RE = re.compile(r"\b(CHN|SEC|EXC|CODE|NAME|DLL)(\d{3})\b")
CXX_CHECK_RE = re.compile(r"lenovo-(chn|sec|exc|code|name|dll)(\d{3})-[a-z0-9-]+")


def parse_rule_ids(text: str, pattern: re.Pattern[str]) -> set[str]:
    return {f"{m.group(1).upper()}{m.group(2)}" for m in pattern.finditer(text)}


def main() -> int:
    if not ANALYZER_RULES.exists():
        print(f"warn: {ANALYZER_RULES} not found, skipping", file=sys.stderr)
    csharp_ids = (parse_rule_ids(ANALYZER_RULES.read_text(encoding="utf-8"),
                                 RULE_ID_RE)
                  if ANALYZER_RULES.exists() else set())

    cxx_ids = parse_rule_ids(MODULE_CPP.read_text(encoding="utf-8"),
                             CXX_CHECK_RE)

    doc_ids = set()
    for md in DOCS_RULES.glob("*.md"):
        doc_ids.update(parse_rule_ids(md.stem.upper(), RULE_ID_RE))

    errors: list[str] = []
    for rule_id in sorted(cxx_ids):
        if rule_id not in doc_ids:
            errors.append(
                f"  MISSING DOCS: {rule_id} is registered in LenovoTidyModule.cpp "
                f"but docs/rules/{rule_id.lower()}.md does not exist"
            )
    for rule_id in sorted(doc_ids):
        if rule_id not in cxx_ids:
            errors.append(
                f"  ORPHAN DOCS: docs/rules/{rule_id.lower()}.md exists but "
                f"rule {rule_id} is not registered"
            )

    print("== Rule coverage ==")
    print(f"  C# AnalyzerRules.md  : {len(csharp_ids):>3} rules "
          f"({', '.join(sorted(csharp_ids)) or '(empty)'})")
    print(f"  C++ registered       : {len(cxx_ids):>3} rules "
          f"({', '.join(sorted(cxx_ids)) or '(empty)'})")
    print(f"  docs/rules/*.md      : {len(doc_ids):>3} pages "
          f"({', '.join(sorted(doc_ids)) or '(empty)'})")

    not_yet_ported = sorted(csharp_ids - cxx_ids)
    if not_yet_ported:
        print("  Not yet ported (informational only):")
        for rid in not_yet_ported:
            print(f"    - {rid}")

    if errors:
        print("\n!! Inconsistencies detected:")
        for err in errors:
            print(err)
        return 1

    print("\nAll good: every registered check has a docs page, and vice versa.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
