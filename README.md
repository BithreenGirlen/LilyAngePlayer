# LilyAngePlayer

何かの閲覧用。

## 前準備

1. 資産ファイルを入手。
    - 自作するか、[リリース](https://github.com/BithreenGirlen/LilyAngePlayer/releases)参照。
2. 資産ファイルを伸展し、脚本以外の素材を抽出。
    - 使い慣れた方法を選んで下さい。
3. 脚本ファイルをJSON形式で吐き出し。
    - [UABEA](https://github.com/BithreenGirlen/UABEA)で吐き出し可能です。

次のような階層構造から成る素材・脚本が整えば準備完了です。
<pre>
Assets
├ ...
└ Naninovel
  ├ ...
  ├ Audio
  │  ├ ...
  │  └ Sfx // 効果音フォルダ
  │     └ ...
  ├ Backgrounds
  │  └ MainBackground // 静画フォルダ
  │     ├ Event
  │     │  ├ ...
  │     │  ├ モル
  │     │  │  ├ chara1001_cg_1_1.png
  │     │  │  └ ...
  │     │  └ ...
  │     └ ...
  ├ ...
  ├ Scripts // 脚本フォルダ
  │  ├ ...
  │  ├ chara1001_201.nani // この形式のファイルを選択
  │  └ ...
  ├ Voice // 音声フォルダ
  │  ├ 1001
  │  │  ├ voice_1001_01_00001.m4a
  │  │  └ ...
  │  └ ...
  └ ...
</pre>

## 再生方法

メニュー欄`File->Open`から`Naninovel/Scripts/charaXXXX_20X.nani`ファイルを選択して下さい。  
上記の相対位置に素材ファイルがあると想定して場面再生を開始します。

## マウス機能

| 入力 | 機能 |
| --- | --- |
| マウスホイール | 拡大・縮小。 |
| 右ボタン + マウスホイール | 文章送り・戻し。 |
| 左ボタンドラッグ | 表示範囲移動。モニタ解像度以上に拡大した場合のみ動作。 |
| 中ボタンクリック | 原寸大表示。 |
| 右ボタン + 中ボタンクリック | 窓枠消去・表示。消去時にはモニタ原点位置に移動。 |
| 右ボタン + 左ボタンクリック | 窓移動。 窓枠消去時のみ動作。|

## キーボード機能

| 入力 | 機能 |
| --- | --- |
| <kbd>Esc</kbd> | 終了。 |
| <kbd>C</kbd> | 文字色黒・白切り替え。 |
| <kbd>T</kbd> | 文章表示・非表示切り替え。 |
| <kbd>∧</kbd> | 前のファイルを開く。 |
| <kbd>∨</kbd> | 次のファイルを開く。 |
| <kbd>＞</kbd> | 文章送り。 |
| <kbd>＜</kbd> | 文章戻し。 |

## 外部ライブラリ
- [JSON for Modern C++ v3.11.3](https://github.com/nlohmann/json/releases/tag/v3.11.3)

## 構築方法
1. `src/deps/CMakeLists.txt`を実行して外部ライブラリを取得。
2. Visual Studioから`LilyAngePlayer.sln`を開く。
3. メニュー欄`ビルド`から`ソリューションのビルド`を選択。

## 追加修正

一部ファイルは配置や名称が脚本指定と異なるので、修正する必要があります。
- chara1023_203.nani
  - `Event/雛菊/雫`階層にある以下の3つのファイルを`Event/雫`に移動。
    - chara1023_cg_3_1.png
    - chara1023_cg_3_2.png
    - chara1023_cg_3_3.png
- chara1032_203.nani
  - `voice_1032_04_00110.m4a`など`_04_`を含むファイル名を`_01_XXXXX`の書式に変更。
