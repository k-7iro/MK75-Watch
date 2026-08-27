# MK75-Watch
> [!IMPORTANT]
> There are no stable releases in this repository, so some features may be gimmicky.

MK75-Watch (It's called Maker Seventy-Five Watch) is a smartwatch/multi-function clock for M5Stack Core2/CoreS3. It does not have a heart rate monitor, but it can be integrated with various modules.

## Purchase Hardware
> [!IMPORTANT]
> - Please note that functionality varies depending on the M5Stack model.
> - Fire will be supported in the future, but is not recommended. Even after support is added, an external RTC will still be required.

| | Basic V2.7 | Fire V2.7 | Core2 (V1.1) | CoreS3 SE | CoreS3 (Lite) | 
| ---- | ---- | ---- | ---- | ---- | ---- |
| Supported status | :x:Not supported | :x:Not supported | :white_check_mark:Supported, Recommended | :warning:Supported, Not Recommended | :white_check_mark:Supported, Recommended |
| PSRAM (Needed) | :x: | :white_check_mark: |:white_check_mark: | :white_check_mark: | :white_check_mark: |
| RTC (Needed) | :x:External | :x:External |:white_check_mark:Internal | :white_check_mark:Internal | :white_check_mark:Internal |
| Touch Screen (Needed) | :x: | :x: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| 3 Buttons (Alternative) | :white_check_mark:Physical | :white_check_mark:Physical  | :white_check_mark:Capacitive | :x: | :x: |
| 6-axis IMU (Recommended) | :x: | :white_check_mark: |:white_check_mark: | :x: | :white_check_mark: |
| 3-axis Compass | :x: | :x: | :x: | :x: | :white_check_mark: |
| Env. Light Sensor | :x: | :x: | :x: | :x: | :white_check_mark: |
| Vibration | :x: | :x: | :white_check_mark: | :x: | :x: |
| Color | Black | Red | White | White | Darkgray |
| Price and Shop link | [$39.90](https://shop.m5stack.com/products/esp32-basic-core-lot-development-kit-v2-7) | [$49.90](https://shop.m5stack.com/products/m5stack-fire-iot-development-kit-psram-v2-7) | [$46.90 (V1.1)](https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit-v1-1) | [$38.90](https://shop.m5stack.com/products/m5stack-cores3-se-iot-controller-w-o-battery-bottom) | [$44.90 (Lite)](https://shop.m5stack.com/products/m5stack-cores3-lite-esp32s3-iot-dev-kit)

- PSRAM is required for screen drawing.
- RTC is required for keeping the time. For models without an RTC, an external RTC can be installed.
- The touchscreen is used for most operations.
- The 3 buttons can be used as an alternative to the touchscreen, but some apps will not work.
- The 6-axis gyro sensor is required for the "tilt to wake" feature.
- The 3-axis compass is required for the compass app (not yet implemented, planned).
- The ambient light sensor is required for automatic screen brightness adjustment.
- Vibration is activated by button touch and when charging.

To use it as a smartwatch, you will also need a [Watch Development Kit w/ Orange Strap (Excluding Core) v1.1](https://shop.m5stack.com/products/watch-development-kit-w-orange-strap-excluding-core-v1-1) (sold separately).

## How to Use

### First boot
On the first boot (when `/wifi.json` is missing), the watch opens **Wi-Fi Setup**.

1. Connect your phone or computer to the Wi-Fi network **MK75-Setup** (no password).
2. A setup page should open by itself (captive portal). If it does not, scan the QR code on the watch, or open http://192.168.10.75/ .
3. Pick an SSID from the list, or type it (needed for hidden networks). Enter the password and tap **Connect**.
4. Watch the watch screen. If it fails, it will return to setup so you can try again.

You can **Skip** from the phone page, or by tapping **Skip / スキップ** on the right side of the watch (Core2: button A also skips). Time can be set later in **Settings → Date and Time**.

To add or change Wi-Fi later, open **Settings → Wi-Fi Setup**. A successful connection is merged into `wifi.json` (other saved networks are kept).

### Clock
- The analog dial is the home screen. Battery is the arc around the dial (cyan while charging).
- Date, time, and battery % are at the top and bottom. Special dates from `special_dates.json` rotate on the bottom line. If a newer firmware is known, **Update Available** is shown there in red.
- Open the app list by touching the left side of the screen (the dial). The list slides in from the right; swipe it vertically and tap an icon.
- Close an app with the red **&lt;** at the top left, or Core2 button A. Button B returns to the clock.

### Apps
| App | What it does |
| ---- | ---- |
| Timer | Countdown. Add one, set minutes:seconds, it rings like the alarm. |
| Alarm | Multiple alarms. Weekday / weekend on or off. Save after editing. Still rings after shutdown (RTC wake). |
| Stopwatch | Five stopwatches. Swipe sideways. Tap to start/stop, hold about 1 second to reset. |
| TrainTime | Next 3 trains/buses from `train.json`. Switch weekday/holiday and clock vs remaining minutes. |
| Random | Dice: 1d2 … 1d10 and 1d100. Tap the number to roll again. |
| Ext.Device | Not implemented yet (placeholder). |
| Settings | Language, power/sleep, Wi-Fi, date/time, display, notices, version. Tap **Save** or changes are lost. |

### Sleep and power
- After idle, the screen sleeps (timing is in **Power Settings**, separately for charging vs battery).
- Short-press the power button to sleep or wake. On battery, tilting the watch (gyro) can also wake it.
- If it sits still for a long time with no timer running, it may shut down to save power. Alarms can still wake it.

### Data files (LittleFS / SD)
At boot, files in the SD card folder `/littlefs/` are copied onto internal LittleFS (same filenames).

| File | Role |
| ---- | ---- |
| `wifi.json` | `{"SSID":"password", ...}`. If this file is missing, Wi-Fi Setup runs. |
| `train.json` | Timetables. `timetable` → station name → `weekdays` / `weekends` → hour (`"6"` … `"23"`) → list of `{"m": minute, "t": type, "d": destination}`. `color` maps type name to `[R,G,B]`. |
| `alarm.json` | Alarm list. Written by the Alarm app. |
| `special_dates.json` | `{"1":{"1":[{"name":"Happy New Year!","color":16398158}]}}` — month → day → list of name + 24-bit color. |

Sample files live in the `data/` folder of this repository.

### Language
Japanese and English. Change it in Settings. For other languages, please open a GitHub Issue.

## Notes
- The M5Stack is not waterproof. Do not take it outside on rainy days.
- It is quite large for a watch (Core2: approximately 5.4 x 5.4 x 2 cm). Be prepared.
- The M5Stack is designed for indoor use. I didn't notice any issues in sunny weather (other than discoloration of the exterior), but please use it at your own risk.
- ~~While the product includes data for printing a cover using a 3D printer, ~~complete waterproofing cannot be guaranteed. Please think of the cover as a way to protect the exterior from scratches and increase the chances of survival in an accident.
- It will take some time to include the cover data due to complicated software licensing issues.

## Acknowledgments
- I used this link as a reference for setting the time: https://github.com/m5stack/M5Unified/blob/master/examples/Basic/Rtc/Rtc.ino
- ~~I used this link for icons not included in the images folder: https://icooon-mono.com/~~
  Currently, the icons distributed on this site are not being used.
- The icons included in the images folder were generated using Arena.ai's AI generators, including Nano Banana (although I only use Nano Banana and Nano Banana Pro).
- I copied some of the source code from [Wikipedia](https://ja.wikipedia.org/wiki/%E3%83%84%E3%82%A7%E3%83%A9%E3%83%BC%E3%81%AE%E5%85%AC%E5%BC%8F). This function simply implements a mathematical formula directly and is therefore not subject to copyright. At least, that's what K-Nana believes.

## Trademarks
- M5Stack is a trademark of M5Stack Technology Co., Ltd.
- Grove is a registered trademark of Seeed Technology Co., Ltd.
- USB is a registered trademark of USB Implementers Forum.
- I2C is a trademark of NXP Semiconductors.
- ESP32 is a trademark of Espressif Systems.
- Other product and company names mentioned herein may be the trademarks of their respective owners.

----

# MK75-Watch（日本語）
> [!IMPORTANT]
> このレポジトリに安定リリースは存在していません。そのため、一部の機能がハリボテだったりします。

MK75-Watch（メイカーセブンティファイブウォッチと読んでください）はM5Stack Core2/CoreS3用スマートウォッチ/多機能小型置時計です。心拍数機能などはありませんが、モジュール等で様々な連携が行えます。

## ハードウェアの購入
> [!IMPORTANT]
> - M5Stackのモデルによって機能が異なります。ご注意ください。
> - Fireは対応予定ですが、非推奨です。また、対応後も使用には外部RTCを取り付ける必要があります。

| | Basic V2.7 | Fire V2.7 | Core2 (V1.1) | CoreS3 SE | CoreS3 (Lite) | 
| ---- | ---- | ---- | ---- | ---- | ---- |
| 対応状況 | :x:非対応 | :x:非対応 | :white_check_mark:対応、推奨 | :warning:対応、非推奨 | :white_check_mark:対応、推奨 |
| PSRAM（必須） | :x: | :white_check_mark: |:white_check_mark: | :white_check_mark: | :white_check_mark: |
| 内蔵RTC（必須） | :x:外付け可能 | :x:外付け可能 |:white_check_mark: | :white_check_mark: | :white_check_mark: |
| タッチスクリーン（推奨） | :x: | :x: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| 3ボタン（代替） | :white_check_mark:物理 | :white_check_mark:物理 | :white_check_mark:静電 | :x: | :x: |
| 6軸ジャイロセンサー（推奨） | :x: | :white_check_mark: |:white_check_mark: | :x: | :white_check_mark: |
| 3軸コンパス | :x: | :x: | :x: | :x: | :white_check_mark: |
| 環境光センサー | :x: | :x: | :x: | :x: | :white_check_mark: |
| バイブレーション | :x: | :x: | :white_check_mark: | :x: | :x: |
| 色 | 黒 | 赤 | 白 | 白 | 暗い灰色 |
| 値段と購入 | [¥6,831（税込）](https://ssci.to/9010) | [¥8,536（税込）](https://ssci.to/9009) | [¥8,976（税込/v1.1）](https://ssci.to/9349) | [¥6,919（税込）](https://ssci.to/9690) | [¥7,986（税込/Lite）](https://ssci.to/10610)

- PSRAMは画面描画に必須です。
- RTCは時刻を保持するために必須です。内蔵されていないモデルでは外付けが可能です。（[基板](https://ssci.to/7308)、[外装](https://ssci.to/8450)）
- タッチスクリーンはほとんどの操作に使用されます。
- 3ボタンはタッチスクリーンの代替として使用できますが、一部のアプリは動作しません。
- 6軸ジャイロセンサーは「傾けて起動させる」機能に必要です。
- 3軸コンパスはコンパスアプリ（未実装、予定）に必要です。
- 環境光センサーは画面の明るさの自動調整に必要です。
- バイブレーションはボタンタッチ・充電時に発動します。

スマートウォッチとして使用するには、別売りの[ウォッチデバイス化キット](https://ssci.to/9492)も必要です。また、別売りの画面保護フィルム（[Basic/Fire用](https://www.amazon.co.jp/dp/B07KF5KWJP)・[Core2用](https://www.amazon.co.jp/dp/B08HMQW367)・[CoreS3用](https://www.amazon.co.jp/dp/B0C4XVTVV8)）も同時に購入することを強くお勧めします。3Dプリンターをお持ちの場合は、カバーを印刷し装着することをお勧めします。

## 使い方

### 初回起動
初回起動時（`/wifi.json` が無いとき）は **Wi-Fi設定** が開きます。

1. スマホやPCを、パスワードなしのWi-Fi **MK75-Setup** に接続します。
2. 設定ページが自動で開きます（キャプティブポータル）。開かないときは時計のQRコードを読むか、http://192.168.10.75/ を開いてください。
3. SSIDを一覧から選ぶか、直接入力します（隠しSSIDは直接入力）。パスワードを入れて **接続** を押します。
4. 時計の画面を見てください。失敗すると設定に戻るので、やり直できます。

スマホのページから **スキップ** するか、時計右下の **Skip / スキップ** をタップしても進めます（Core2はボタンAでもスキップ）。時刻はあとから **設定 → 日付と時刻** で合わせられます。

あとからWi-Fiを足したり変えたりするときは **設定 → Wi-Fi設定** を使います。接続に成功したSSIDは `wifi.json` に追記され、他の登録はそのまま残ります。

### 時計画面
- アナログ文字盤がホームです。電池残量は文字盤の弧（充電中はシアン）です。
- 時刻・日付・電池%は上下に出ます。`special_dates.json` の記念日は下の行で順番に表示されます。新しいファームが分かっているときは、そこに赤い **Update Available** も出ます。
- アプリ一覧は画面左（文字盤）をタッチすると右から出ます。縦にスワイプしてアイコンをタップします。
- アプリを閉じるときは左上の赤い **&lt;**、またはCore2のボタンAです。ボタンBで時計に戻ります。

### アプリ
| アプリ | 内容 |
| ---- | ---- |
| タイマー | カウントダウン。分:秒でセット。鳴り方はアラームと同じです。 |
| アラーム | 複数登録。平日/休日のオンオフ。編集したら保存してください。電源オフ中もRTCで鳴ります。 |
| ストップWt | 5本。横スワイプ。タップで開始/停止、約1秒長押しでリセット。 |
| 交通時刻表 | `train.json` から次の3本。平日/休日と、時刻表示/残り分を切り替えできます。 |
| ランダム | 1d2〜1d10と1d100。出た数字をタップすると再ロール。 |
| 外部デバイス | 未実装です（プレースホルダ）。 |
| 設定 | 言語、電源/スリープ、Wi-Fi、日付と時刻、画面、通知、バージョン。**保存** を押さないと消えません。 |

### スリープと電源
- 操作しないでいると画面がスリープします（時間は **電源設定** で、充電時と電池時それぞれ）。
- 電源ボタン短押しでスリープ/解除。電池動作時は傾けても起きることがあります（ジャイロ）。
- 長く置いたままでタイマーが無いと、節電のためシャットダウンすることがあります。アラームでは起動します。

### データファイル（LittleFS / SD）
起動時、SDカードの `/littlefs/` にあるファイルが内蔵LittleFSへ同名でコピーされます。

| ファイル | 役割 |
| ---- | ---- |
| `wifi.json` | `{"SSID":"password", ...}`。このファイルが無いとWi-Fi設定が走ります。 |
| `train.json` | 時刻表。`timetable` → 駅名 → `weekdays` / `weekends` → 時（`"6"` … `"23"`）→ `{"m": 分, "t": 種別, "d": 行先}` の配列。`color` は種別名 → `[R,G,B]`。 |
| `alarm.json` | アラーム一覧。アラームアプリが書き込みます。 |
| `special_dates.json` | `{"1":{"1":[{"name":"Happy New Year!","color":16398158}]}}` — 月 → 日 → 名前と24bit色の配列。 |

リポジトリの `data/` にサンプルがあります。

### 言語
日本語と英語。設定から切り替えます。ほかの言語が必要な場合はGitHubのIssueで知らせてください。

## 注意点
- M5Stackに防水機能は全くありません。雨の日に外に持ち出さないでください。
- 腕時計としてはかなり大きい（Core2:約5.4x5.4x2cm）です。覚悟してください。
- M5Stackは通常室内で使われることを前提に設計されているはずです。晴れていれば（外装の色あせを除き）問題は見当たりませんでしたが、自己責任で持ち運んでください。
- ~~カバーを3Dプリンターで印刷できるデータが含まれておりますが~~、カバーを付けても完全な防水は保証できません。外装に傷がつかないようにしたり、アクシデントから助かる確率を上げるものだと思ってください。
- カバーデータ同梱は仕様ソフトのライセンス関係が面倒くさいのでしばらくかかります。

## 謝辞
- 時刻合わせはこちらを参考にさせていただきました。https://github.com/m5stack/M5Unified/blob/master/examples/Basic/Rtc/Rtc.ino
- ~~imagesフォルダに含まれないアイコンはこちらを使用しています。https://icooon-mono.com/~~
  現状、このサイトで配布されているアイコンは使用されていません。
- imagesフォルダに含まれるアイコンはArena.aiでNano Bananaをはじめとする（というかNano BananaとNano Banana Proしか使ってない）生成AIにより生成されました。
- [Wikipedia](https://ja.wikipedia.org/wiki/%E3%83%84%E3%82%A7%E3%83%A9%E3%83%BC%E3%81%AE%E5%85%AC%E5%BC%8F)からソースコードの一部をコピーさせていただきました。このソースコードは数式を創意工夫なしにそのまま実装したもののため、著作権の対象にはなりません。少なくともK-Nanaはそう考えています。

## 商標
- M5StackはM5Stack Technology Co., Ltd.の商標です。
- GroveはSeeed Technology Co., Ltd.の商標です。
- USBはUSB Implementers Forumの商標です。
- I2CはNXP Semiconductors.の商標です。
- ESP32はEspressif Systems.の商標です。
- 本書に記載されているその他の製品名および会社名は、それぞれの所有者の商標である場合があります。