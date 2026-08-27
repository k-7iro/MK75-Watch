# Changelog
Update version numbers are in the format `[Year].[Month]` or `[Year].[Month].[Minor Update]`. Some months may be missing. Most updates include minor fixes that are not listed here. The versions listed below are also available from M5Burner. Other development versions are only available from GitHub.

# Ver 26.8
- Replaced the first-boot Wi-Fi setup: captive portal (the phone should open the page by itself), URL-encoded SSID/password (spaces and symbols no longer break setup), hidden SSID typing, skip, Japanese/English page, and retry from Settings.
- Wi-Fi Setup no longer wipes other saved networks when you add one.
- After first-boot Wi-Fi setup, timetable / alarm / special-date files are loaded without needing a reboot.
- Added a "How to Use" section to the README.

# Ver 26.7
- Added a "Random App" feature.
- Updated the update detection mechanism to support HTTPS and fixed a bug that prevented updates from being received.
- Fixed a bug that prevented navigation back from certain settings screens.
- Added a setting to toggle the display of battery voltage.
- Due to various circumstances, the CoreS3 version of V26.7 is not available on M5Burner. Please download it from GitHub or use an older version.

# Ver 26.6
- You can now set the volume and vibration settings for alarm and timer notifications.
- You can now change the display brightness in the settings.
- You can now set the maximum charging current and maximum charging voltage from the settings.
- Due to various circumstances, the CoreS3 version of V26.6 is not available on M5Burner. Please download it from GitHub or use an older version.

# Ver 26.5
- Reduced power consumption by partially putting the ESP32 into light sleep mode.
- The ZeroDial design has been slightly adjusted.
- Sleep and wake functions can now be activated with a short press of the power button.
- Notifications are now displayed when an update is available.
- Due to various circumstances, the CoreS3 version of V26.5 is not available on M5Burner. Please download it from GitHub or use an older version.

# Ver 26.4.1
- Added a new theme, "ZeroDial," experimentally. You can revert to the traditional watch face style in the settings.
- Fixed a bug where automatic time setting would enter an infinite loop when connecting to Wi-Fi.
- Fixed a bug where setting the time would sometimes prevent the next alarm from sounding.
- Fixed a bug where initial settings were not applied after Wi-Fi setup.
- ~~Due to various circumstances, the CoreS3 version of V26.4.1 is not available on M5Burner. Please download it from GitHub or use an older version.~~
- For some reason, the M5Burner version was not available regardless of whether it was Core2 or CoreS3.

# Ver 26.4
- You can now configure the sleep time and external port power output in the settings.
- Fixed a bug where the timetable displayed incorrect times.
- Fixed a bug where the day of the week did not change when the year was changed.
- Improved the readability of the QR code during setup.
- Due to various circumstances, the CoreS3 version of V26.4 is not available on M5Burner. Please download it from GitHub or use an older version.

# Ver 26.3
- Processing speed for some graphics has increased.
- Fixed an issue where the system would not shut down properly when left idle.
- The date time can now be set manually. You can adjust the time down to the minute. For precise adjustments down to the second, please continue to use the internet to set the time as before.
- Added version infomation in Settings.
- Due to various circumstances, the CoreS3 version of V26.3 is not available on M5Burner. Please download it from GitHub or use an older version.

# Ver 26.2.1
- Adjusted to make touch response more responsive.

# Ver 26.2
- Adjustments to ensure full compatibility with CoreS3.
- Added language switching. Currently, only Japanese and English are supported. If you require other languages, please let us know via [Issue](https://github.com/k-7iro/MK75-Watch/issues).
- Adjusted so that an alarm sounds even when the system shuts down. It will automatically start at the alarm time.
- Added a feature that automatically shuts down the device when it is left stationary.
- The dial's circle has been smoothed.

# Ver 26.1
- First release.
- The current Wi-Fi setup is unstable and unuser-friendly. A more stable and user-friendly Wi-Fi setup will be added in the future.

----

# 変更ログ（日本語）
バージョン番号は`[年].[月]`または`[年].[月].[マイナーアップデート]`のように表記されます。一部の月は欠番となることがあります。ほとんどのアップデートはここに書かれていない細かなバグ修正を含みます。以下にリストアップされたバージョンはM5Burnerでも使用可能です。ほかの開発バージョンはGitHubからでのみダウンロードできます。

# Ver 26.7
- ランダムアプリを追加しました。
- アップデート検知がHTTPSに対応し、アップデートを受け取れないバグを修正しました。
- 設定の一部項目にて戻ることができないバグを修正しました。
- 電池電圧の表示のオンオフを設定で変更できるようになりました。
- 諸事情により、V26.7のCoreS3バージョンはM5Burnerで提供できません。GitHubからダウンロードするか古いバージョンを使用してください。

# Ver 26.6
- アラームやタイマーの通知のボリューム及び振動の有無が設定できるようになりました。
- ディスプレイの輝度を設定で変更できるようになりました。
- 設定から最大充電電流と最大充電電圧を設定できるようになりました。
- 諸事情により、V26.6のCoreS3バージョンはM5Burnerで提供できません。GitHubからダウンロードするか古いバージョンを使用してください。

# Ver 26.5
- ESP32を部分的にライトスリープさせることにより消費電力の削減を図りました。
- ZeroDialのデザインが微調整されました。
- 電源ボタンの短押しでスリープ及びスリープ解除ができるようになりました。
- アップデートが来た時に通知が来るようになりました。
- 諸事情により、V26.5のCoreS3バージョンはM5Burnerで提供できません。GitHubからダウンロードするか古いバージョンを使用してください。

# Ver 26.4.1
- 試験的に新しいテーマ「ZeroDial」を追加しました。設定で従来スタイルの文字盤に戻せます。
- 自動時刻合わせで、Wi-Fiの接続で無限ループに入ってしまうバグを修正しました。
- 時刻設定をすると次のアラームが鳴らないことがあるバグを修正しました。
- Wi-Fiセットアップ後に初期設定が反映されないバグを修正しました。
- ~~諸事情により、V26.4.1のCoreS3バージョンはM5Burnerで提供できません。GitHubからダウンロードするか古いバージョンを使用してください。~~
- なぜかCore2、CoreS3関係なくM5Burner版の提供はありませんでした。

# Ver 26.4
- 設定でスリープするまでの時間や外部ポートの電源出力を設定できるようになりました。
- 時刻表で誤った時刻が表示されるバグを修正しました。
- 年を変更しても曜日が変わらないバグを修正しました。
- セットアップ時のQRコードの表示を読み取りやすく変更しました。
- 諸事情により、V26.4のCoreS3バージョンはM5Burnerで提供できません。GitHubからダウンロードするか古いバージョンを使用してください。

# Ver 26.3
- 一部のグラフィックスのパフォーマンスを向上させました。
- 動かさずに置いているときに自動シャットダウンが発生しないことがある問題を修正しました。
- 日付と時刻を手動設定できるようにしました。手動で設定する場合、時刻は分単位まで調整可能です。秒単位の調整が必要な場合、これまで通りインターネット経由での時刻合わせを行ってください。
- バージョン情報を設定に追加しました。
- 諸事情により、V26.4のCoreS3バージョンはM5Burnerで提供できません。GitHubからダウンロードするか古いバージョンを使用してください。

# Ver 26.2.1
- タッチへの反応を改善しました。

# Ver 26.2
- CoreS3に対応するための調整を行いました。
- 言語切り替え機能を追加しました。現在日本語と英語のみ対応です。ほかの言語が必要な場合は[Issue](https://github.com/k-7iro/MK75-Watch/issues)でお知らせください。
- シャットダウンされた時でもアラームが鳴るようにしました。アラーム時刻で自動起動します。
- 動かさずに置いているときに自動シャットダウンが発生するようにしました。
- 文字盤の円をスムーズにしました。

# Ver 26.1
- 最初のリリースです。
- 現在のWi-Fiセットアップは操作が難しく、なおかつ不安定です。今後より操作が簡単で安定したものに置き換わるでしょう。