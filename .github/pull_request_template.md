## Summary

<!-- 何を、なぜ。バグ由来なら Issue / 再現手順へのリンクを。 -->

## Test plan

> **CI は ripgrep lint だけで C++ をビルドしない。** ビルドの正しさはここに書く実行結果だけが担保する。
> 「通るはず」ではなく、実際に走らせた出力から書くこと。詳細は [AGENTS.md](../AGENTS.md#-pr-を出す前に必ずローカルで通すこと)。

- [ ] `build_editor.bat` でビルド成功（UE 5.7 / `Result: Succeeded`、新規 warning なし）
- [ ] 触った領域の `Python/tests/verify_*.py` を live editor に対して実行し PASS
      <!-- 例: verify_safe_create_bt_graph_runtime_node.py 6/6 PASS -->
- [ ] verify script が無い領域を触った場合、`Python/tests/verify_<feature>.py` を追加した
- [ ] CI lint (`lint-primitive-bypass`) が green

<!-- C++ を一切触らない PR (docs のみ等) は上の 3 つを「N/A: docs only」と書いて消してよい -->

## Primitive I/O Layer チェック

- [ ] ハンドラから禁止 API を直呼びしていない（`FBlueprintEditorUtils::CompileBlueprint` / `FGraphNodeCreator<>`）
- [ ] `SPIRROW_PRIMITIVE_BYPASS` を追加した場合、理由を本文で説明した
      <!-- v0.11.1 時点でハンドラ側の bypass は 0 件。増やすなら要説明。 -->
