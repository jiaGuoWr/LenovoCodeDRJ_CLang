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

/// Walk up from `start_dir` looking for a directory that contains
/// `compile_commands.json`. Returns the **directory** that contains the
/// file (which is what `clang-tidy -p` expects), not the file path itself.
///
/// `candidate_subdirs` is the ordered list of relative sub-directories to
/// probe at every level (an empty entry means "the current directory
/// itself"); first hit wins. Callers typically pass
/// [`crate::config::Config::compile_commands_search_paths`]. The list is
/// sourced from the LSP client (e.g. `lenovoTidy.compileCommandsSearchPaths`
/// in VS Code settings) so there is a single source of truth.
///
/// Returns `None` if no database is found anywhere up to the filesystem
/// root.
pub fn find_compile_db<S: AsRef<str>>(
    start_dir: &Path,
    candidate_subdirs: &[S],
) -> Option<PathBuf> {
    let mut cur: Option<&Path> = Some(start_dir);
    while let Some(dir) = cur {
        for sub in candidate_subdirs {
            let sub = sub.as_ref();
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
    use crate::config::default_compile_commands_search_paths;
    use std::fs;
    use tempfile::TempDir;

    fn touch_db(dir: &Path) {
        fs::create_dir_all(dir).unwrap();
        fs::write(dir.join("compile_commands.json"), b"[]").unwrap();
    }

    /// Tests use the production default list so any regression in the
    /// default ordering or entries is caught here.
    fn default_dirs() -> Vec<String> {
        default_compile_commands_search_paths()
    }

    #[test]
    fn finds_in_build_subdirectory() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("build"));

        let found = find_compile_db(&src, &default_dirs())
            .expect("should find db in ../build");
        assert_eq!(found, tmp.path().join("build"));
    }

    #[test]
    fn finds_in_out_subdirectory_when_no_build() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src").join("nested");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("out"));

        let found = find_compile_db(&src, &default_dirs())
            .expect("should walk up and find db in ../../out");
        assert_eq!(found, tmp.path().join("out"));
    }

    #[test]
    fn returns_none_when_no_database_anywhere() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();

        // No compile_commands.json anywhere.
        assert!(find_compile_db(&src, &default_dirs()).is_none());
    }

    #[test]
    fn build_wins_over_out_at_same_level() {
        // Order matters: when both build/ and out/ exist, build/ is preferred
        // because it appears first in the default list.
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("build"));
        touch_db(&tmp.path().join("out"));

        let found = find_compile_db(&src, &default_dirs()).unwrap();
        assert_eq!(found, tmp.path().join("build"));
    }

    #[test]
    fn finds_in_workspace_root_itself() {
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(tmp.path()); // file lives at the workspace root

        let found = find_compile_db(&src, &default_dirs())
            .expect("should accept root-level db");
        assert_eq!(found, tmp.path());
    }

    #[test]
    fn honours_custom_search_path_order() {
        // When the caller passes a user-supplied list, that list must be
        // honoured even if it contradicts the production default (e.g. a
        // user who prefers `out/` over `build/`, or invents `.build/`).
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join("build"));
        touch_db(&tmp.path().join("out"));

        let custom: Vec<String> = vec!["out".into(), "build".into()];
        let found = find_compile_db(&src, &custom).unwrap();
        assert_eq!(found, tmp.path().join("out"));
    }

    #[test]
    fn honours_custom_path_not_in_default() {
        // A user who builds into `.build/` (Meson convention) can add it
        // to the config and discovery picks it up even though it is not in
        // the default list.
        let tmp = TempDir::new().unwrap();
        let src = tmp.path().join("src");
        fs::create_dir_all(&src).unwrap();
        touch_db(&tmp.path().join(".build"));

        let custom: Vec<String> = vec![".build".into()];
        let found = find_compile_db(&src, &custom).unwrap();
        assert_eq!(found, tmp.path().join(".build"));

        // And without it, discovery finds nothing.
        let empty: Vec<String> = vec!["build".into()];
        assert!(find_compile_db(&src, &empty).is_none());
    }
}
