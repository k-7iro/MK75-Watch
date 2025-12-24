# MK75-Watch
> [!IMPORTANT]
> このレポジトリに安定リリースは存在していません。そのため、一部の機能がハリボテだったりします。

M5Stack Core2/CoreS3用スマートウォッチ/多機能置時計です。心拍数機能などはありませんが、モジュール等で様々な連携が行えます。

## 購入
> [!IMPORTANT]
> - M5Stackのモデルによって機能が異なります。ご注意ください。
> - 開発者のK-Nanaは2025年12月15日現在Core2 V1.1のみ所持しております。Core2以外のモデルでのみ発生するバグは報告されても修正が遅れる可能性があります。
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
使って慣れろ。

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