//! Internal diagnostic representation that bridges clang-tidy YAML and the LSP
//! `Diagnostic` type. Keeping our own struct lets us add helpers (e.g. offset
//! to line/column conversion) without leaking tower-lsp into the parser.

use tower_lsp::lsp_types::{Diagnostic, DiagnosticSeverity, Position, Range};

#[derive(Debug, Clone)]
pub struct LenovoDiagnostic {
    pub source_file: String,
    pub file_offset: u64,
    pub code: String,
    pub message: String,
    pub severity: String,
    pub file_uri_hint: String,
}

impl LenovoDiagnostic {
    /// Convert into a tower-lsp `Diagnostic`. Position is computed from byte
    /// offsets against the supplied document text. When the offset is past the
    /// end of the buffer (e.g. preprocessor-generated) we collapse to start.
    pub fn to_lsp(&self, document_text: &str) -> Diagnostic {
        let (line, character) = offset_to_position(document_text, self.file_offset as usize);
        let position = Position {
            line: line as u32,
            character: character as u32,
        };

        Diagnostic {
            range: Range {
                start: position,
                end: position,
            },
            severity: Some(map_severity(&self.severity)),
            code: Some(tower_lsp::lsp_types::NumberOrString::String(
                self.code.clone(),
            )),
            code_description: None,
            source: Some("lenovo-tidy".to_owned()),
            message: self.message.clone(),
            related_information: None,
            tags: None,
            data: None,
        }
    }
}

fn map_severity(level: &str) -> DiagnosticSeverity {
    match level.to_ascii_lowercase().as_str() {
        "error" | "fatal" => DiagnosticSeverity::ERROR,
        "warning" => DiagnosticSeverity::WARNING,
        "note" | "remark" => DiagnosticSeverity::INFORMATION,
        _ => DiagnosticSeverity::WARNING,
    }
}

fn offset_to_position(text: &str, offset: usize) -> (usize, usize) {
    let clamped = offset.min(text.len());
    let mut line = 0usize;
    let mut last_line_start = 0usize;
    for (idx, byte) in text.as_bytes().iter().enumerate().take(clamped) {
        if *byte == b'\n' {
            line += 1;
            last_line_start = idx + 1;
        }
    }
    let column = clamped - last_line_start;
    (line, column)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn offset_zero_maps_to_origin() {
        let (l, c) = offset_to_position("abc\ndef", 0);
        assert_eq!((l, c), (0, 0));
    }

    #[test]
    fn offset_in_second_line() {
        let (l, c) = offset_to_position("abc\ndef", 5);
        assert_eq!((l, c), (1, 1));
    }

    #[test]
    fn offset_past_end_clamps() {
        let (l, c) = offset_to_position("abc", 100);
        assert_eq!((l, c), (0, 3));
    }
}
