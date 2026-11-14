# Changelog

このプロジェクトの主な変更を記録します。書式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に、バージョン番号は [Semantic Versioning](https://semver.org/lang/ja/) に従います。

- MAJOR: `config.json` の形式に互換性のない変更、または対象ハードウェアの変更
- MINOR: 機能の追加
- PATCH: 不具合の修正、計算精度の改善

要件が固まるまでは `0.x` とします。

## [Unreleased]

### 変更

- マニュアルのフッターのブログ記事へのリンクを、公開した紹介記事 (日本語版・英語版) に差し替えました。英語のページからは英語版の記事を開きます。

## [0.2.0] - 2026-11-14

公開リポジトリでの最初のリリースです。対象は M5Stack Basic V2.6 です。

### Added
- `config.json` の `home.ipinfo_token` (任意): 設定すると ipinfo.io への問い合わせにアクセストークンを付けます
- `LICENSE` (MIT)、`THIRD_PARTY_NOTICES.md`、`config.example.json`、英語版 README
- ユーザーマニュアル (日本語・英語、GitHub Pages) と、ブラウザから書き込める Web Flasher
- GitHub Actions による CI (テスト、ビルド、マニュアルのビルド) とリリースの自動化
- `tools/package_firmware.py`: 一括イメージとアプリケーション単体のイメージを作成

### Changed
- `tools/gen_photomaps.py` は Pillow があれば使い、なければ macOS の `sips` を使います

## 0.1.0 - 2026-11-14 (開発版の履歴)

非公開の開発版です。このリポジトリにはタグと Release はありません。対象は M5Stack Basic V2.6 です。

### Added
- 正距円筒図法の世界地図に昼・夜・薄明 (市民、航海、天文) とグレイラインを描画
- 地図の表示を 2 種類から選択 (ボタン B): 写真風 (NASA Blue Marble / Earth at Night) と薄明の塗り分け
- 重ね描きの切り替え (ボタン B 長押し): Maidenhead グリッド、月 (月直下点と満ち欠け)
- 現地時刻と UTC の表示、Wi-Fi 経由の NTP 同期 (ボタン A で再同期、既定は 6 時間ごと)
- 自局の次の日の出・日の入りまでのカウントダウン、極夜・白夜の表示
- 詳細画面 (ボタン C): 薄明・日の出・日の入り・南中の時刻、昼の長さ、太陽の高度と方位、赤緯、均時差、月の輝面比、グリッドロケーター
- microSD の `/grayline/config.json` による設定 (Wi-Fi、自局、地図の中心、タイムゾーン、NTP、画面の明るさ)
- 自局位置の自動設定: config → GeoIP (ipinfo.io) → 既定 (東京)
- 画面の自動減光 (夜間・バッテリー動作時) とボタンでの一時点灯
- 電池残量・給電状態・直前の再起動要因の表示
- ファームウェアのバージョン表示 (起動時と詳細画面)
- 単体テスト (`pio test -e native`) と実機の画面確認スクリプト (`tools/sweep_locations.py`)

[Unreleased]: https://github.com/waganeko-dev/m5-grayline/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/waganeko-dev/m5-grayline/releases/tag/v0.2.0
