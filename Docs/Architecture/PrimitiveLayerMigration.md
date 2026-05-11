# Primitive I/O Layer — Migration & Authoring Guide

> Issue: [#14](https://github.com/SpirrowGames/spirrow-unrealwise/issues/14)
> Status: introduced v0.11.0
> Audience: contributors writing or modifying SpirrowBridge command handlers

---

## Why this layer exists

Over the past year a class of bugs has repeated in SpirrowBridge handlers:

| Incident | Root cause |
|---|---|
| Issue #11 (`set_struct_array_property` swallowing nested `FStructProperty`) | Each handler reimplemented its own leaf-only `if (FIntProperty) ... else if (FFloatProperty) ...` chain and the `FStructProperty` branch was missing. |
| `SafeCompileBlueprint` crash (2026-04-02, `EXCEPTION_ACCESS_VIOLATION` after component add) | `GeneratedClass == SkeletonGeneratedClass` was reconciled ad hoc in 20+ call sites, with drift. |
| BT 2-layer creation bug | The "`FGraphNodeCreator` → `Finalize` → runtime `NodeInstance` with `GraphNode` as Outer" invariant was enforced only by prompt-level docs (`Docs/Prompts/BT_Graph_Based_Node_Creation_Prompt.md` v2). |

The shared shape: **handlers call UE internals directly, re-implement an invariant in each spot, drift, miss a case, fail silently or AV.**

The primitive I/O layer (`SpirrowBridgePrimitives`) collapses those invariants into a single audited implementation that handlers must go through.

---

## Layer model

```
┌─────────────────────────────────────────────────────────┐
│ Command handlers (~148 in SpirrowBridge*Commands.cpp)   │
│   - Parse JSON params, validate, call primitives,       │
│     marshal response. No direct UE internals for the    │
│     patterns we've already paid for in bugs.            │
└──────────────────────────┬──────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│ SpirrowBridgePrimitives namespace                       │
│   - SafeCompileBlueprint           (Issue: AV repro)    │
│   - SetPropertyValueAtAddress      (Issue #11)          │
│   - SafeCreateBTGraphAndRuntimeNode<T,R> (Issue #14)    │
└──────────────────────────┬──────────────────────────────┘
                           │
                           ▼
            UE Reflection / Editor API
```

`FSpirrowBridgeCommonUtils` continues to host generic helpers (JSON, validation, asset lookup, etc.). Its `SafeCompileBlueprint` / `SetPropertyValueAtAddress` members are now thin wrappers that delegate to the namespace — kept for source compatibility with existing handlers. New handlers should call the namespace functions directly.

---

## Authoring rules

### 1. Use primitives for the patterns the lint enforces

CI rejects (see `.github/workflows/lint-primitive-bypass.yml`) direct calls to:

- `FBlueprintEditorUtils::CompileBlueprint` → use `SpirrowBridgePrimitives::SafeCompileBlueprint`
- `FGraphNodeCreator<...>` → use `SpirrowBridgePrimitives::SafeCreateBTGraphAndRuntimeNode<TGraphNode, TRuntimeBase>`

These are the only two patterns enforced today. The set grows when (and only when) a new bug class proves it's needed.

### 2. Sub-function decomposition

Primitives must stay readable. Hard rule: **if a primitive body exceeds ~200 lines, decompose into sub-functions named for the type they handle.** `SetPropertyValueAtAddress` already follows this pattern internally (per-type branches: Bool, Numeric, String, Object, Struct, Array, Map, Set). When you add a primitive, plan the decomposition before writing the dispatch table — not after.

### 3. Boy scout rule (no migration sprint)

There is no dedicated migration PR for the existing ~148 handlers. Instead:

> When you touch a handler file for any reason — bug fix, feature, refactor — and a primitive exists for the pattern that file uses, migrate that file's callers to the primitive as part of the same change.

Files you do not touch stay as-is. This avoids main destabilization and lets primitive coverage grow organically with normal development.

### 4. `SPIRROW_PRIMITIVE_BYPASS` escape hatch

When direct API use is genuinely justified (primitive impl itself, narrow Editor-only case primitive doesn't cover yet, etc.), suppress the lint by placing a comment on the same line:

```cpp
FGraphNodeCreator<UBehaviorTreeGraphNode_SimpleParallel> NodeCreator(*BTGraph); // SPIRROW_PRIMITIVE_BYPASS: boy-scout migration deferred (Issue #14)
```

The comment must:

1. Include the literal token `SPIRROW_PRIMITIVE_BYPASS`.
2. Give a reason — vague reasons get rejected in review.

Bypass is for the rare exception, not the common case.

---

## Current primitives

### `SafeCompileBlueprint(UBlueprint*)`

Resolves the `GeneratedClass == SkeletonGeneratedClass` AV pattern. Picks between a full compile (with skeleton regen) and a Dirty-only path depending on the blueprint's compile state. Replaces the pre-2026-04-02 ad-hoc reconciliation across 20+ call sites.

### `SetPropertyValueAtAddress(FProperty*, void*, JsonValue, FString& OutErr)`

Recursive property writer. Handles all FProperty subtypes including nested `FStructProperty`, `FArrayProperty`, `FMapProperty`, `FSetProperty`, enums, object/class refs; falls back to `FProperty::ImportText_Direct` for unclaimed string inputs. Fixed Issue #11 in v0.10.2.

### `SafeCreateBTGraphAndRuntimeNode<TGraphNode, TRuntimeBase>(...)`

Template primitive that enforces the BT 2-layer creation invariant:

1. `FGraphNodeCreator<TGraphNode>` → `CreateNode()`.
2. `NewObject<TRuntimeBase>(GraphNode, RuntimeClass, Name, RF_Transactional)` — the runtime node's Outer is **always** the GraphNode.
3. `GraphNode->NodeInstance` / `GraphNode->ClassData` wired.
4. `NodeCreator.Finalize()` runs after wiring so `AllocateDefaultPins` sees `NodeInstance`.

Explicit instantiations in `SpirrowBridgeCommonUtils.cpp` cover:

- `<UBehaviorTreeGraphNode_Composite, UBTCompositeNode>`
- `<UBehaviorTreeGraphNode_SimpleParallel, UBTCompositeNode>`
- `<UBehaviorTreeGraphNode_Task, UBTTaskNode>`
- `<UBehaviorTreeGraphNode_SubtreeTask, UBTTaskNode>`

Add new instantiations to that file when a new graph-node/runtime-base pair is introduced.

---

## Future primitive candidates (recorded, not committed)

These are observed duplication / risk hotspots that **do not** have a confirmed bug yet. They live here so the next person who hits the same pattern has a starting point — they are not promises and are not on a roadmap.

### `SafeCaptureSceneAndReadback`

Observed in PR #13 (`HandleTakePIEPOVScreenshot`, v0.10.1/0.10.3). The 100-plus-line setup for `ASceneCapture2D` + `USceneCaptureComponent2D` ShowFlags/CaptureSource/FOV + `UTextureRenderTarget2D` + `CaptureScene` → `FlushRenderingCommands` → `ReadPixels` → PNG → alpha forced opaque is likely to be re-implemented for a future editor-side screenshot command and for Thirdy's automated visual capture. Candidate scope when that second caller appears.

### Live Coding compatibility note for static caches

PR #13 used a function-local `static TWeakObjectPtr<ASceneCapture2D>` to cache the spawned capture actor. Observed safe pattern (UE 5.7):

- Plain `static` storage may be re-initialized by Live Coding patches.
- `TWeakObjectPtr` wrapped over the `static` survives correctly because UObject lifecycle is independent of the static slot — a re-initialized null `TWeakObjectPtr` just triggers a re-spawn on the next call (cache miss, no crash).
- Avoid plain `static UObject*` or raw pointers in primitive helpers — they can dangle across Live Coding patches.

This pattern should be reused when a future primitive needs cached actor/object state.

---

## Adding a new primitive — checklist

1. **Anchor on a real bug** — at least one Issue / PR review note describing the silent failure or crash that motivated it. No predictive primitives.
2. **Header forward decl** in `SpirrowBridgeCommonUtils.h`, inside `namespace SpirrowBridgePrimitives`.
3. **Implementation** in `SpirrowBridgeCommonUtils.cpp`, near the existing primitives. Keep sub-functions per type / per branch if dispatch is non-trivial.
4. **Explicit instantiations** in the same `.cpp` if templated.
5. **Verify script** under `Python/tests/verify_<feature>.py` exercising the primitive end-to-end through at least one handler.
6. **Lint update** in `.github/workflows/lint-primitive-bypass.yml` if a new direct-API pattern should now be forbidden (forbid only after the primitive ships and at least one handler is migrated).
7. **CHANGELOG entry** in `Docs/CHANGELOG.md`.
8. **Update this document** — extend the "Current primitives" section and remove the entry from "Future candidates" if it was listed.
