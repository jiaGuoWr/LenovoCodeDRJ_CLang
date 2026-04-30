//! Filesystem discovery helpers for `compile_commands.json`.
//!
//! When the LSP client doesn't supply `compileCommandsDir` in
//! `initializationOptions`, the driver falls back to walking up the
//! directory tree from the source file looking for the database in any of
//! a small set of conventional sub-directories. This mirrors what VS
//! Code's extension does on the client side, but keeps a single source of
//! truth on the server so every client (VS Code, VS 2022, Neovim, plain
//! `lenovo-tidy-lsp`) gets the same behaviour for free.

use std::path::{Path, PathBuf};

/// Sub-directories searched at every level on the way up from the source
/// file. An empty entry means "the current directory itself". Order
/// matters: the first hit wins.
const CANDIDATE_SUBDIRS: &[&str] = &[
    "build",
    "out",
    "cmake-build-debug",
    "cmake-build-release",
    "cmake-build-relwithdebinfo",
    ".vscode",
    "", // workspace root itself
];

/// Walk up from `start_dir` looking for a directory that contains
/// `compile_commands.json`. Returns the **directory** that contains the
/// file (which is what `clang-tidy -p` expects), not the file path itself.
///
/// Returns `None` if no database is found anywhere up to the filesystem
/// root.
pub fn find_compile_db(start_dir: &Path) -> Option<PathBuf> {
    let mut cur: Option<&Path> = Some(start_dir);
    while let Some(dir) = cur {
        for sub in CANDIDATE_SUBDIRS {
            let candidate_dir = if sub.is_empty() {
                dir.to_path_buf()
            } else {
                dir.join(sub)
            };
            if candidate_dir.join("compile_commands.json").is_file() {
                return Some(candidate_dir);
            }
        }
        cur = dir.parent();
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    use tempfile::TempDir;

    fn touch_db(dir: &Path) {
        fs::create_dir_all(dir).unwrap();
        fs::write(dir.join("compile_commands.json"), b"[]").unwrap();
    }

    #[test]
    fn finds_in_build_subdirectory() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("build"));

        let found = find_compile_db(&src).expect("should find db in ../build");
        assert_eq!(found, tmp.path().join("build"));
    }

    #[test]
    fn finds_in_out_subdirectory_when_no_build() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src").join("nested");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("out"));

        let found = find_compile_db(&src).expect("should walk up and find db in ../../out");
        assert_eq!(found, tmp.path().join("out"));
    }

    #[test]
    fn returns_none_when_no_database_anywhere() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();

        // No compile_commands.json anywhere.
        assert!(find_compile_db(&src).is_none());
    }

    #[test]
    fn build_wins_over_out_at_same_level() {
        // Order matters: when both build/ and out/ exist, build/ is preferred
        // because it appears first in CANDIDATE_SUBDIRS.
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("build"));
        touch_db(&tmp.path().join("out"));

        let found = find_compile_db(&src).unwrap();
        assert_eq!(found, tmp.path().join("build"));
    }

    #[test]
    fn finds_in_workspace_root_itself() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(tmp.path()); // file lives at the workspace root

        let found = find_compile_db(&src).expect("should accept root-level db");
        assert_eq!(found, tmp.path());
    }
}
