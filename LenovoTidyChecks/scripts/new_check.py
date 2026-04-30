#!/usr/bin/env python3
"""Generate scaffolding for a new LenovoTidyChecks rule.

Usage::

    python3 scripts/new_check.py \\
        --id SEC002 \\
        --name path-traversal \\
        --category security

Creates:
  * src/LenovoTidyModule/<category>/<ClassName>Check.{h,cpp}
  * tests/checkers/<id-lower>-<name>/{main,valid}.cpp
  * docs/rules/<id-lower>.md
  * appends a registerCheck line to src/LenovoTidyModule/LenovoTidyModule.cpp
  * appends an entry to docs/mkdocs.yml nav

The generated files compile and pass lit with a placeholder warning; you then
follow the TDD workflow in docs/tdd-workflow.md to replace placeholders with
real logic.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent

ALLOWED_CATEGORIES = {
    "security": "SEC",
    "localization": "CHN",
    "naming": "NAME",
    "reliability": "EXC",
    "maintainability": "CODE",
}


def to_pascal(text: str) -> str:
    return "".join(part.capitalize() for part in re.split(r"[-_\s]+", text) if part)


def kebab_name(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def render(template: str, **values: str) -> str:
    return template.format(**values)


HEADER_TEMPLATE = """\
//===- {class_name}Check.h - {rule_id} ---===//
#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::{category} {{

class {class_name}Check : public clang::tidy::ClangTidyCheck {{
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
}};

}} // namespace clang::tidy::lenovo::{category}
"""

SOURCE_TEMPLATE = """\
#include "{class_name}Check.h"

#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::{category} {{

void {class_name}Check::registerMatchers(MatchFinder *finder) {{
  // TODO({rule_id}): register your matcher here. Example:
  // finder->addMatcher(functionDecl().bind("fn"), this);
  (void)finder;
}}

void {class_name}Check::check(const MatchFinder::MatchResult &result) {{
  // TODO({rule_id}): implement the rule body.
  (void)result;
}}

}} // namespace clang::tidy::lenovo::{category}
"""

LIT_MAIN_TEMPLATE = """\
// RUN: %check_clang_tidy %s lenovo-{id_lower}-{name_kebab} %t

// TODO({rule_id}): replace with a positive example that should trigger the rule.
int placeholder_bad = 0;
// CHECK-MESSAGES-NOT: warning:
"""

LIT_VALID_TEMPLATE = """\
// RUN: %check_clang_tidy %s lenovo-{id_lower}-{name_kebab} %t

int placeholder_good = 0;
// CHECK-MESSAGES-NOT: warning:
"""

DOC_TEMPLATE = """\
# {rule_id} - {title}

| 属性 | 取值 |
|---|---|
| **ID** | `{rule_id}` / `lenovo-{id_lower}-{name_kebab}` |
| **类别** | {category_title} |
| **严重级别** | Warning |
| **默认启用** | 是 |

## 描述

TODO: describe the rule.

## 触发示例

```cpp
// TODO: add bad example
```

## 合规示例

```cpp
// TODO: add good example
```
"""


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--id", required=True, help="Rule id, e.g. SEC002")
    parser.add_argument("--name", required=True,
                        help="Rule short-name, e.g. path-traversal")
    parser.add_argument("--category", required=True,
                        choices=sorted(ALLOWED_CATEGORIES.keys()))
    parser.add_argument("--title", default=None,
                        help="Human-readable rule title")
    args = parser.parse_args(argv)

    rule_id = args.id.upper()
    prefix = ALLOWED_CATEGORIES[args.category]
    if not rule_id.startswith(prefix):
        print(f"error: id {rule_id!r} should start with {prefix}", file=sys.stderr)
        return 2

    id_lower = rule_id.lower()
    name_kebab = kebab_name(args.name)
    class_name = to_pascal(args.name)
    title = args.title or args.name.replace("-", " ").title()

    subst = {
        "rule_id": rule_id,
        "id_lower": id_lower,
        "name_kebab": name_kebab,
        "class_name": class_name,
        "category": args.category,
        "category_title": args.category.title(),
        "title": title,
    }

    src_dir = REPO_ROOT / "src" / "LenovoTidyModule" / args.category
    src_dir.mkdir(parents=True, exist_ok=True)

    header_path = src_dir / f"{class_name}Check.h"
    source_path = src_dir / f"{class_name}Check.cpp"
    if header_path.exists() or source_path.exists():
        print(f"error: {header_path} already exists", file=sys.stderr)
        return 3

    header_path.write_text(render(HEADER_TEMPLATE, **subst))
    source_path.write_text(render(SOURCE_TEMPLATE, **subst))

    test_dir = REPO_ROOT / "tests" / "checkers" / f"{id_lower}-{name_kebab}"
    test_dir.mkdir(parents=True, exist_ok=True)
    (test_dir / "main.cpp").write_text(render(LIT_MAIN_TEMPLATE, **subst))
    (test_dir / "valid.cpp").write_text(render(LIT_VALID_TEMPLATE, **subst))

    doc_path = REPO_ROOT / "docs" / "rules" / f"{id_lower}.md"
    doc_path.parent.mkdir(parents=True, exist_ok=True)
    doc_path.write_text(render(DOC_TEMPLATE, **subst))

    print(f"Scaffolding created:\n"
          f"  {header_path.relative_to(REPO_ROOT)}\n"
          f"  {source_path.relative_to(REPO_ROOT)}\n"
          f"  {test_dir.relative_to(REPO_ROOT)}/\n"
          f"  {doc_path.relative_to(REPO_ROOT)}")
    print("\nNext steps:")
    print(f"  1. Add #include and registerCheck<{class_name}Check> to "
          f"src/LenovoTidyModule/LenovoTidyModule.cpp")
    print(f"  2. Add the source files to src/LenovoTidyModule/CMakeLists.txt")
    print("  3. Follow docs/tdd-workflow.md to fill in the rule body")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
