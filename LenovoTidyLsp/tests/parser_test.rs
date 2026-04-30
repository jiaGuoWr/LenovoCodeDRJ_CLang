//! Integration tests at the parser layer. We do not require a real
//! clang-tidy binary here; instead we feed canned --export-fixes YAML and
//! assert the parser produces the expected diagnostic shape.

use std::path::Path;

#[path = "../src/diagnostic.rs"]
mod diagnostic;

#[path = "../src/parser.rs"]
mod parser;

const SAMPLE_YAML: &str = r#"---
MainSourceFile: 'a.cpp'
Diagnostics:
  - DiagnosticName:    lenovo-sec001-hardcoded-sensitive
    DiagnosticMessage:
      Message:        "variable 'password' may contain hardcoded sensitive information"
      FilePath:       'a.cpp'
      FileOffset:     12
    Level:            Warning
  - DiagnosticName:    lenovo-chn001-chinese-comments
    DiagnosticMessage:
      Message:        "comment contains Chinese characters"
      FilePath:       'a.cpp'
      FileOffset:     30
    Level:            Warning
  - DiagnosticName:    lenovo-name001-naming-convention
    DiagnosticMessage:
      Message:        "class 'foo_bar' should use PascalCase (e.g. 'FooBar')"
      FilePath:       'a.cpp'
      FileOffset:     50
    Level:            Warning
...
"#;

#[test]
fn parser_returns_three_diagnostics() {
    let diags = parser::parse(SAMPLE_YAML.as_bytes(), "file:///a.cpp");
    assert_eq!(diags.len(), 3);
    assert_eq!(diags[0].code, "lenovo-sec001-hardcoded-sensitive");
    assert_eq!(diags[1].code, "lenovo-chn001-chinese-comments");
    assert_eq!(diags[2].code, "lenovo-name001-naming-convention");
}

#[test]
fn parser_handles_empty_input() {
    let diags = parser::parse(b"", "file:///x.cpp");
    assert!(diags.is_empty());
}

#[test]
fn parser_recovers_from_malformed_yaml() {
    let diags = parser::parse(b"::not yaml::", "file:///x.cpp");
    assert!(diags.is_empty());
}

#[test]
#[ignore = "requires real clang-tidy + LenovoTidyChecks plugin"]
fn end_to_end_smoke() {
    use std::process::Command;
    let _ = Path::new("tests/fixtures/violations.cpp");
    let exists = Command::new("clang-tidy-18")
        .arg("--version")
        .output()
        .is_ok();
    if !exists {
        eprintln!("skipping: clang-tidy-18 not on PATH");
    }
}
