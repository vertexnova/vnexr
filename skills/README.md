# vnexr Skills

Task playbooks for working in vnexr. Each folder holds a self-contained
`SKILL.md` (YAML frontmatter plus a short body) that any AI coding tool or human
can read. Structure matches the VertexNova `skills/` pattern used by sibling
libraries (vnerhi / vnegfx).

## Skills

| Skill | Enforces | Fires when |
|-------|----------|------------|
| [vne-coding-style](vne-coding-style/SKILL.md) | Naming, formatting, modern C++ | Writing or editing C++ |
| [plain-ascii-authoring](plain-ascii-authoring/SKILL.md) | ASCII-only in new and edited text | Producing any code, comment, doc, or message |
| [vne-header-hygiene](vne-header-hygiene/SKILL.md) | Include order, header self-containment | Editing includes or creating headers |
| [vne-build-verify](vne-build-verify/SKILL.md) | Format, build, run on Ubuntu / Android | Before finishing a source change |
| [vne-testing](vne-testing/SKILL.md) | GoogleTest layout, determinism | Adding or changing tests |
| [vne-xr-session](vne-xr-session/SKILL.md) | Host-owned device and swapchains | XR session, sample, or swapchain work |
| [vne-xr-android](vne-xr-android/SKILL.md) | APK, manifest, adb, Galaxy XR | Android packaging or device runs |

## Using these skills

Claude Code: symlink this directory into `.claude/skills/`:

```bash
mkdir -p .claude/skills
for s in skills/*/; do ln -sfn "../../$s" ".claude/skills/$(basename "$s")"; done
```

## Self-check

```bash
rg -nP "[^\x00-\x7F]" skills
```

A clean run prints nothing.
