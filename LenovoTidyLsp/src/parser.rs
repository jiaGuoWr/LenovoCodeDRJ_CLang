//! Parses the `--export-fixes` YAML output of clang-tidy into our internal
//! `LenovoDiagnostic` structs. We deliberately keep the schema small; only the
//! fields actually consumed by the LSP layer are deserialised.

use serde::Deserialize;

use crate::diagnostic::LenovoDiagnostic;

#[derive(Debug, Deserialize)]
struct ExportFixes {
    #[serde(rename = "Diagnostics", default)]
    diagnostics: Vec<DiagnosticEntry>,
}

#[derive(Debug, Deserialize)]
struct DiagnosticEntry {
    #[serde(rename = "DiagnosticName")]
    name: String,
    #[serde(rename = "DiagnosticMessage")]
    message: DiagnosticMessage,
    #[serde(rename = "Level", default)]
    level: Option<String>,
}

#[derive(Debug, Deserialize)]
struct DiagnosticMessage {
    #[serde(rename = "Message", default)]
    message: String,
    #[serde(rename = "FilePath", default)]
    file_path: String,
    #[serde(rename = "FileOffset", default)]
    file_offset: u64,
}

pub fn parse(yaml: &[u8], file_uri_text: &str) -> Vec<LenovoDiagnostic> {
    if yaml.is_empty() {
        return Vec::new();
    }
    let parsed: Result<ExportFixes, _> = serde_yaml::from_slice(yaml);
    let entries = match parsed {
        Ok(p) => p.diagnostics,
        Err(err) => {
            tracing::warn!(?err, "failed to parse clang-tidy yaml, returning empty");
            return Vec::new();
        }
    };

    entries
        .into_iter()
        .filter_map(|entry| {
            // Map clang-tidy severity to LSP severity-ish levels stored in our
            // intermediate diag struct.
            let severity_label = entry
                .level
                .as_deref()
                .unwrap_or("warning")
                .to_ascii_lowercase();

            // We rely on the file_offset only as a pre-conversion handle; the
            // server layer translates byte-offsets to line/column using the
            // open document text.
            Some(LenovoDiagnostic {
                source_file: entry.message.file_path,
                file_offset: entry.message.file_offset,
                code: entry.name,
                message: entry.message.message,
                severity: severity_label,
                file_uri_hint: file_uri_text.to_owned(),
            })
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    const SAMPLE: &str = r#"---
MainSourceFile: 'a.cpp'
Diagnostics:
  - DiagnosticName:    lenovo-sec001-hardcoded-sensitive
    DiagnosticMessage:
      Message:        "variable 'password' may contain hardcoded sensitive information"
      FilePath:       'a.cpp'
      FileOffset:     12
    Level:            Warning
...
"#;

    #[test]
    fn parses_one_diagnostic() {
        let diags = parse(SAMPLE.as_bytes(), "file:///a.cpp");
        assert_eq!(diags.len(), 1);
        assert_eq!(diags[0].code, "lenovo-sec001-hardcoded-sensitive");
        assert!(diags[0].message.contains("password"));
        assert_eq!(diags[0].severity, "warning");
    }

    #[test]
    fn empty_yaml_yields_empty() {
        assert!(parse(b"", "file:///a.cpp").is_empty());
    }
}
