# 開発者向けの情報

[README](../README.ja.md) に戻る

この文書では、m5-grayline のビルド、テスト、デバッグに使うシリアルコマンドとツール、バージョンの付け方を説明します。

## 開発の流れ

ブランチ、PR、リリースの規則は [AGENTS.md](../AGENTS.md) にまとめています (AI エージェントと人間の共通の規則です)。要点は次のとおりです。

- main には直接 push せず、PR を経由します。PR は squash でマージします。
- ブランチ名は `feature/`、`fix/`、`docs/`、`chore/` のいずれかで始め、続けて小文字とハイフンで内容を書きます (例: `feature/moon-overlay`)。
- clone したら、最初に次のコマンドで pre-push フックを有効にします。規則に合わないブランチ名や main への push を、手元で止めます。

```bash
git config core.hooksPath .githooks
```

## ビルドと書き込み

[PlatformIO](https://platformio.org/) を使います。

```bash
pio run -t upload
```

書き込み先のポートを指定する場合は、`--upload-port <port>` を付けます。`<port>` は本体のシリアルポートです (例: Linux では `/dev/ttyUSB0`、macOS では `/dev/cu.usbserial-XXXX`、Windows では `COM3`)。

Wi-Fi に接続できない場合や NTP に失敗した場合は、ビルド時刻を仮の時刻として "NO SYNC" と表示します。ボタン A で再同期します。

## シリアルコマンド (115200bps)

シリアルモニター (`pio device monitor` など) から次の文字を送ると、本体を操作できます。

- `C`: 設定を表示します (パスワードと ipinfo.io のトークンは伏せ字)。
- `H`: 自局の位置を自動 (GeoIP) に切り替えます。`config.json` から `home` の緯度経度と `map.center_lon` を削除します。
- `L<緯度>,<経度>,<UTC との差 (分)>,<タイムゾーン名>,<地名>`: 再起動するまで、一時的に自局の位置とタイムゾーンを変更します (保存はしません)。例: `L51.5074,-0.1278,0,GMT,London`
- `S`: 画面のスクリーンショットを送信します (`tools/screenshot.py` が使用します)。
- `b` / `c`: ボタン B / C と同じ動作をします。
- `B`: ボタン B の長押しと同じ動作をします。
- `w`: ボタンによる一時点灯と同じ動作をします。

USB シリアル変換 IC (CH9102) の自動リセット回路により、PC の環境によってはシリアルポートを開いたときに本体がリセットされます。

## ツール

`tools/screenshot.py` と `tools/sweep_locations.py` は pyserial を使います。PlatformIO に同梱の Python を使うか、`pip install pyserial` でインストールしてください。

- `tools/gen_landmask.py`: Natural Earth の陸地データ (`ne_50m_land.geojson`、パブリックドメイン) から `src/landmask.h` (320×160、1bit、約 6.4KB) を生成します。
- `tools/gen_photomaps.py`: 昼夜の地球の画像 (NASA、パブリックドメイン、Wikimedia Commons の "Whole world - land and oceans.jpg" と "The earth at night.jpg") から `src/photomaps.h` (RGB565、各 100KB) を生成します。画像の読み込みと縮小には、Pillow があれば Pillow を、なければ macOS の `sips` を使います。
- `tools/screenshot.py`: シリアル経由で画面を PNG に保存します。起動完了を待ってから取得します。第 3 引数で、先にキーを送信できます (`c` で詳細画面)。
- `tools/version.py`: ビルド時に Git からバージョンを埋め込みます (後述)。
- `tools/package_firmware.py`: `pio run` のあとに実行し、一括イメージ (`0x0` に書き込む `*-full.bin`) とアプリケーション単体のイメージ (`0x10000` に書き込む `*-app.bin`) を `out/` に作ります。ブートローダーなどのアドレスは `pio project metadata` から読み取ります。`--web docs/public` を付けると、Web Flasher 用に `docs/public/firmware/<version>/` と `manifest.json` も作ります。

```bash
python tools/screenshot.py <port> out.png c
```

## マニュアル (docs/)

ユーザーマニュアルは [Astro Starlight](https://starlight.astro.build/) で作り、GitHub Pages で公開します。Node.js と npm が必要です。ページは `docs/src/content/docs/` にあり、日本語が既定 (ルート)、英語が `en/` です。画面の画像は `docs/src/assets/images/` にあります。

```bash
cd docs
npm ci
npm run dev      # 編集しながら確認 (http://localhost:4321/m5-grayline/)
npm run build    # docs/dist/ に出力
npm run preview  # ビルド結果を確認
```

Web Flasher をローカルで確認するには、現在のソースからファームウェアをビルドし、`docs/public/firmware/` に配置します。このフォルダは Git の管理対象外です。

```bash
cd docs
npm run firmware:local
```

ブログ記事へのリンクの URL は `docs/src/site.config.mjs` の `BLOG_URL` で設定します。

## CI とリリース

- `.github/workflows/ci.yml`: push と pull request で、単体テスト、ファームウェアのビルド、マニュアルのビルドを実行します。
- `.github/workflows/release.yml`: `v*` のタグを push すると、テストとビルドのあと、一括イメージとアプリケーション単体のイメージを添付した GitHub Release を作ります。本文は `CHANGELOG.md` の該当バージョンの節です。続いて、ファームウェアを同梱したマニュアルを GitHub Pages にデプロイします。

リリースの手順は次のとおりです。

1. `CHANGELOG.md` の `[X.Y.Z]` の節に日付を入れ、コミットします。
2. `git tag vX.Y.Z` でタグを付け、`git push origin vX.Y.Z` で push します。

## テスト

### 単体テスト (PC 上で実行)

太陽・月の計算、時刻の書式、地名の省略など、本体に依存しない処理を、PlatformIO の native 環境と Unity でテストします。

```bash
pio test -e native
```

- `test/test_astro`: 太陽赤緯と均時差 (至点・分点)、各地の日の出・日の入り (暦との比較)、全地点・全季節での自己整合性 (算出した時刻の太陽高度が目標値と一致し、順序が正しいこと)、極夜・白夜・薄明が終わらない地域、極点、日付変更線、さまざまな時差 (-12:00 から +14:00、+5:45 など)、次の日の出・日の入りの選択、月の満ち欠け、グリッドロケーター。
- `test/test_textfit`: 地名の省略 (UTF-8 の文字を途中で切らないことを含む)。

新しい地点やパターンは、`test/test_astro/test_main.cpp` の `PLACES` に追加すると、全テストの対象になります。

### 実機での画面確認

テスト用の地点を順に実機へ設定し、地図画面と詳細画面のスクリーンショットを 1 枚 (`<out_dir>/sheet.png`) にまとめます。

```bash
python tools/sweep_locations.py <port> out/
```

地点は `tools/sweep_locations.py` の `LOCATIONS` で設定します。文字のはみ出し、描画の消し残し、"--:--"・Polar night・Midnight sun の表示が正しいかを確認します。

## バージョン

[Semantic Versioning](https://semver.org/lang/ja/) に従い、Git タグ `vX.Y.Z` を付けます。

- MAJOR: `config.json` の形式に互換性のない変更、または対象ハードウェアの変更
- MINOR: 機能の追加
- PATCH: 不具合の修正、計算精度の改善

要件が固まるまでは `0.x` とします。

ビルド時に `tools/version.py` が `git describe` の値をファームウェアに埋め込みます。値は起動時の画面とシリアル出力、詳細画面に表示されます。タグがない場合や、タグより後のコミットからビルドした場合は、バージョンの末尾に "+" が付きます。

`config.json` には形式のバージョン (`"version"`) があります。互換性のない変更をするときは `src/config.h` の `CONFIG_VERSION` を上げ、`loadConfig()` で古い形式を移行します。

変更内容は [CHANGELOG.md](../CHANGELOG.md) の `[Unreleased]` に記録します。
