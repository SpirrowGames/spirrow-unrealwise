# End-user templates for Spirrow-UnrealWise

> **For**: UE 5.x developers using Spirrow-UnrealWise MCP in their game project (e.g. via Claude Code or Claude Desktop).
>
> **Not for**: contributors editing the Spirrow-UnrealWise codebase itself — see `.claude/skills/` and `Docs/DEV_CHEATSHEET.md` at the repo root for that.

This directory is the **canonical source** for Skills, a Cheatsheet, and a CLAUDE.md section that help end-users get more out of Spirrow-UnrealWise. Copy them into your own UE project to enable.

---

## Contents

| File | Where to install in your project | Purpose |
|---|---|---|
| `skills/ue-level-bootstrap/SKILL.md` | `.claude/skills/ue-level-bootstrap/SKILL.md` | New `.umap` + WorldSettings + save (typical map-creation workflow) |
| `skills/ue-blueprint-scaffold/SKILL.md` | `.claude/skills/ue-blueprint-scaffold/SKILL.md` | New BP + components + BeginPlay + compile |
| `skills/ue-hud-bootstrap/SKILL.md` | `.claude/skills/ue-hud-bootstrap/SKILL.md` | UMG Widget BP + layout + viewport display |
| `skills/ue-input-bootstrap/SKILL.md` | `.claude/skills/ue-input-bootstrap/SKILL.md` | Enhanced Input (IA + IMC + bind + default) |
| `SPIRROW_CHEATSHEET.md` | `Docs/SPIRROW_CHEATSHEET.md` (or wherever your project's docs live) | 25 meta-tools × 157 commands quick reference |
| `CLAUDE_SECTION.md` | append into your project's `CLAUDE.md` | Usage rules: meta-tool pattern, help-first discipline, retry strategy, LSB switching |

---

## Quick install (Windows / Bash)

From the root of your UE project, with `spirrow-unrealwise` cloned alongside it (adjust paths as needed):

```bash
# Skills
mkdir -p .claude/skills
cp -r ../spirrow-unrealwise/templates/end-user/skills/ue-level-bootstrap .claude/skills/
cp -r ../spirrow-unrealwise/templates/end-user/skills/ue-blueprint-scaffold .claude/skills/
cp -r ../spirrow-unrealwise/templates/end-user/skills/ue-hud-bootstrap .claude/skills/
cp -r ../spirrow-unrealwise/templates/end-user/skills/ue-input-bootstrap .claude/skills/

# Cheatsheet
mkdir -p Docs
cp ../spirrow-unrealwise/templates/end-user/SPIRROW_CHEATSHEET.md Docs/

# CLAUDE.md section — manually append the contents of CLAUDE_SECTION.md to your CLAUDE.md
cat ../spirrow-unrealwise/templates/end-user/CLAUDE_SECTION.md >> CLAUDE.md
```

---

## After install

- Restart Claude Code so it picks up the new skills.
- `/ue-level-bootstrap` etc. are now invocable.
- Your CLAUDE.md tells Claude to use `help()` first, compile BPs, save after WorldSettings changes, etc.

---

## Customizing for your project

These are templates — fork them. Common per-project tweaks:

- **Path conventions**: Skills default to `/Game/Blueprints` for BPs, `/Game/UI` for widgets, `/Game/Maps` for levels, `/Game/Input` for IAs/IMCs. If your project uses different folders (e.g. `/Game/<ProjectName>/Blueprints/`), edit the `path` defaults in each SKILL.md.
- **Naming prefixes**: Skills suggest `BP_`, `WBP_`, `IA_`, `IMC_`. Change if your project uses different conventions.
- **Parent-class shortcuts**: Add project-specific shortcuts to `ue-blueprint-scaffold` (e.g. `parent_class: BP_TrapBase` for a `TrapxTrap`-style game).

Once customized, your project's `.claude/skills/` is yours — don't sync back to this template directory unless the change is generally useful.

---

## Versioning

These templates are validated against:
- **Spirrow-UnrealWise**: v0.9.5 (25 MCP tools, 157 commands)
- **UE**: 5.7

If you upgrade Spirrow-UnrealWise to a newer version, re-pull this directory and diff against your `.claude/skills/` to pick up improvements.

---

## Reporting issues

If a skill fails or a command listed in the cheatsheet doesn't exist anymore, file an issue at https://github.com/SpirrowGames/spirrow-unrealwise/issues with:
- Spirrow-UnrealWise version (`git log -1` in the repo)
- The skill / command name
- The error message you got
