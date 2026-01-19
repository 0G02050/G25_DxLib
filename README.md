# ColdVault Protocol
**オフライン環境専用・高セキュリティパスワード生成システム**


---

## 🛡️ Project Overview (概要)

**ColdVault** は、「完全オフライン（Air-Gap）」をコンセプトとしたパスワード管理ソリューションです。

インターネットから物理的に遮断された **Windows PC** でパスワードを生成し、暗号化されたQRコードを介して **iPhone** へ安全に転送します。

> **Why ColdVault?**
> 中小企業やスタートアップなど、「専任のセキュリティ担当者がいない」環境での利用を想定しています。
> 高額なサーバー導入や複雑な初期設定は一切不要。「インストールするだけ」で、低コストに強固なセキュリティ環境を構築できます。

---

## 📸 System Workflow (仕組み)

PC側で生成し、スマホ側で受け取る「一方通行」のセキュリティ設計です。

| 🖥️ Generator (PC) | 📱 Scanner (iOS) |
| :--- | :--- |
| **Windows / C++ / DxLib** | **iPhone / Swift / SwiftUI** |
| 1. マスターキーとヒントを入力 | 1. 専用アプリでスキャン |
| 2. パスワード生成＆QR化 | 2. オフラインで即時復号 |
| 3. **ネット接続一切不要** | 3. クリップボードへコピー |


---

## ✨ Key Features (主な機能)

### 🔒 1. Complete Air-Gap Security
Wi-FiやBluetoothは一切使用しません。画面の「光（QRコード）」のみでデータを転送するため、外部からのハッキングは不可能です。

### 🛠️ 2. Customizable Guide Text (New!)
入力ボックスの「入力例（ヒント）」を、運用ルールに合わせて自由に書き換えられます。
* **個人利用:** `Amazon` `Twitter` などのサービス名を表示
* **組織利用:** `部署コード` `フォルダ名` などを表示し、チーム内の入力ミスを防止

### ⚡ 3. High-Speed Encryption
生成されたQRコードは、本プロジェクト専用のアプリでのみ解読可能です。万が一画面を盗み見られても、データの中身は漏洩しません。

---

## 🛠️ Tech Stack (技術構成)

Windows(C++) と iOS(Swift) という異なるプラットフォームを連携させるクロスプラットフォーム開発です。

| Category | Technology | Usage |
| :--- | :--- | :--- |
| **Windows App** | **C++** | 高速処理、メモリ管理、DxLib (GUI) |
| **iOS App** | **Swift / SwiftUI** | AVFoundation (Camera) |
| **Algorithm** | **AES-256** | 堅牢な暗号化ロジック |
| **IDE** | **Visual Studio / Xcode** | 開発環境 |

---

## 🚀 Usage Scenarios (活用シーン)

### 👤 For Personal (個人利用)
* **Seed:** 自分だけの合言葉
* **Hint:** `Webサービス名` + `ID`
* **Result:** サイトごとに異なる強力なパスワードを生成

### 🏢 For SMBs / Teams (中小企業・チーム利用)
* **Seed:** チームの共通キー
* **Hint:** `プロジェクト名` + `共有フォルダ名`
* **Result:** サーバー不要・管理者不要で、安全に機密ファイルのパスワードを共有

---

## 🤝 Special Thanks

This project was developed with AI assistance.
* **Technical Partner:** Gemini (Google DeepMind)
