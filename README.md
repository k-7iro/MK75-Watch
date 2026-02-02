# MK75-Watch
> [!IMPORTANT]
> There are no stable releases in this repository, so some features may be gimmicky.

This is a smartwatch/multi-function clock for M5Stack Core2/CoreS3. It does not have a heart rate monitor, but it can be integrated with various modules.

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
WIP

## Notes
- The M5Stack is not waterproof. Do not take it outside on rainy days.
- It is quite large for a watch (Core2: approximately 5.4 x 5.4 x 2 cm). Be prepared.
- The M5Stack is designed for indoor use. I didn't notice any issues in sunny weather (other than discoloration of the exterior), but please use it at your own risk.
- ~~While the product includes data for printing a cover using a 3D printer, ~~complete waterproofing cannot be guaranteed. Please think of the cover as a way to protect the exterior from scratches and increase the chances of survival in an accident.
- It will take some time to include the cover data due to complicated software licensing issues.

## Acknowledgments
- I used this link as a reference for setting the time: https://github.com/m5stack/M5Unified/blob/master/examples/Basic/Rtc/Rtc.ino
- I used this link for icons not included in the images folder: https://icooon-mono.com/
- The icons included in the images folder were generated using LMArena's AI generators, including Nano Banana (although I only use Nano Banana and Nano Banana Pro).

# 日本語
> [!IMPORTANT]
> このレポジトリに安定リリースは存在していません。そのため、一部の機能がハリボテだったりします。

M5Stack Core2/CoreS3用スマートウォッチ/多機能置時計です。心拍数機能などはありませんが、モジュール等で様々な連携が行えます。

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
後日記載

## 注意点
- M5Stackに防水機能は全くありません。雨の日に外に持ち出さないでください。
- 腕時計としてはかなり大きい（Core2:約5.4x5.4x2cm）です。覚悟してください。
- M5Stackは通常室内で使われることを前提に設計されているはずです。晴れていれば（外装の色あせを除き）問題は見当たりませんでしたが、自己責任で持ち運んでください。
- ~~カバーを3Dプリンターで印刷できるデータが含まれておりますが~~、カバーを付けても完全な防水は保証できません。外装に傷がつかないようにしたり、アクシデントから助かる確率を上げるものだと思ってください。
- カバーデータ同梱は仕様ソフトのライセンス関係が面倒くさいのでしばらくかかります。

## 謝辞
- 時刻合わせはこちらを参考にさせていただきました。https://github.com/m5stack/M5Unified/blob/master/examples/Basic/Rtc/Rtc.ino
- imagesフォルダに含まれないアイコンはこちらを使用しています。https://icooon-mono.com/
- imagesフォルダに含まれるアイコンはLMArenaでNano Bananaをはじめとする（というかNano BananaとNano Banana Proしか使ってない）生成AIにより生成されました。