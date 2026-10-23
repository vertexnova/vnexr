# vnexr Skills

Shared VertexNova conventions and visualization playbooks live in the
[vneskills](https://github.com/vertexnova/vneskills) submodule at
[vneskills/](vneskills/). Install [pstack](https://github.com/cursor/plugins/tree/main/pstack)
for generic rigor, then type `/vne-mode` for visualization work.

```bash
git submodule update --init skills/vneskills
```

## Shared skills (submodule)

See [vneskills/README.md](vneskills/README.md): `vne-mode`, style, headers,
testing, build-verify, frame, picture-check, cross-repo, gpu-benchmark,
backend-matrix.

## Local domain skills

| Skill | Enforces | Fires when |
|-------|----------|------------|
| [vne-xr-session](vne-xr-session/SKILL.md) | Host-owned device and swapchains | XR session, sample, or swapchain work |
| [vne-xr-android](vne-xr-android/SKILL.md) | APK, manifest, adb, Galaxy XR | Android packaging or device runs |

## Using these skills

Claude Code: symlink shared + local skills into `.claude/skills/`:

```bash
mkdir -p .claude/skills
for s in skills/vneskills/skills/*/; do
  ln -sfn "../../../$s" ".claude/skills/$(basename "$s")"
done
for s in skills/*/; do
  name=$(basename "$s")
  [ "$name" = "vneskills" ] && continue
  ln -sfn "../../$s" ".claude/skills/$name"
done
```

## Self-check

```bash
rg -nP "[^\x00-\x7F]" skills
```

A clean run prints nothing (submodule content is already ASCII-checked upstream).
