/*
    [ MK75-Watch HTMLs ] by K-Nana
*/

#pragma once
#include <pgmspace.h>
#include <Arduino.h>

const String Error404 PROGMEM = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";

const String rawHeader_part1 PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: ";
const String rawHeader_part2 PROGMEM = "\r\n\r\n";

const char rawWiFiForm_part1[] PROGMEM = R"HTML(<!DOCTYPE html><html><head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><style>body{margin:0;font-family:sans-serif;background:linear-gradient(90deg,#308ec9,#7e2dc4);min-height:100vh}.container{max-width:480px;margin:12px auto;padding:16px 20px 28px;background:#FFF;border-radius:10px;box-shadow:rgba(34,34,34,.5) 0 0 15px}h1{font-size:1.4em;margin:0 0 .4em}label{display:block;margin:.8em 0 .3em;font-weight:bold}.textbox,.dropdown{width:100%;box-sizing:border-box;padding:8px;border-radius:3px;border:2px solid #ccc;font-size:16px}.button{width:100%;height:40px;margin-top:12px;border-radius:6px;border:2px solid #2d98ca;background:#68cffe;box-shadow:rgba(34,34,34,.5) 1px 1px 2px;font-size:16px}.button:active{background:#60c1ee;box-shadow:inset rgba(34,34,34,.5) 1px 1px 2px}.button.skip{background:#eee;border-color:#999}.muted{color:#555;font-size:.9em}.ok{color:#080}.err{color:#c00}.lang{float:right}.ja,.en{display:none}body.ja .ja,body.en .en{display:block}body.ja span.ja,body.en span.en{display:inline}</style><title>MK75-Watch Setup</title></head><body class="ja"><div class="container"><div class="lang"><button type="button" onclick="document.body.className='ja'">日本語</button><button type="button" onclick="document.body.className='en'">EN</button></div><h1><span class="ja">Wi-Fi設定</span><span class="en">Wi-Fi Setup</span></h1>)HTML";

const char rawWiFiForm_sent[] PROGMEM = R"HTML(<p class="ok"><span class="ja">送信しました。失敗したときは時計の画面を見て、もう一度試してください。</span><span class="en">Your request has been sent. If it fails, check the MK75-Watch screen and try again.</span></p></div><script>if((navigator.language||'').indexOf('ja')!==0)document.body.className='en';</script></body></html>)HTML";

const char rawWiFiForm_part2[] PROGMEM = R"HTML(<p class="ja">時刻合わせにインターネットを使います。SSIDとパスワードを入れて接続を押してください。入力した内容は、消すまで時計に保存されます。</p><p class="en">The MK75-Watch uses the Internet to synchronize its time. Enter the SSID and password, then press Connect. They stay saved until you delete them.</p><label><span class="ja">SSID（一覧）</span><span class="en">SSID (list)</span></label><select id="ssid" class="dropdown"><option value="">--</option>)HTML";

const char rawWiFiForm_part3[] PROGMEM = R"HTML(</select><label><span class="ja">SSID（直接入力・隠れたネットワーク用）</span><span class="en">SSID (type manually / hidden network)</span></label><input type="text" id="ssid_manual" class="textbox" autocomplete="off"><label><span class="ja">パスワード</span><span class="en">Password</span></label><input type="password" id="pass" class="textbox"><label class="muted"><input type="checkbox" onclick="document.getElementById('pass').type=this.checked?'text':'password'"> <span class="ja">パスワードを表示</span><span class="en">Show password</span></label><button class="button" type="button" onclick="connect();"><span class="ja">接続</span><span class="en">Connect</span></button><button class="button skip" type="button" onclick="skip();"><span class="ja">スキップ（あとで設定で時刻合わせ）</span><span class="en">Skip (set the time later in Settings)</span></button><p class="muted ja">複数のWi-Fiを登録できます。初期設定ではまず1件です。追加はSDカードの wifi.json か、設定のWi-Fiから行えます。</p><p class="muted en">You can register more than one network. Initial setup saves one. Add more later with wifi.json on the SD card, or from Settings.</p></div><script>if((navigator.language||'').indexOf('ja')!==0)document.body.className='en';function connect(){var manual=document.getElementById('ssid_manual').value.trim();var ssid=manual||document.getElementById('ssid').value;var pass=document.getElementById('pass').value;if(!ssid){alert(document.body.className==='ja'?'SSIDを入力してください':'Please enter an SSID');return;}location.href='/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);}function skip(){location.href='/connect?skip=1';}</script></body></html>)HTML";

String WiFiSetForm(bool sent, String WiFiList) {
    String WiFiForm = FPSTR(rawWiFiForm_part1);
    if (sent) {
        WiFiForm += FPSTR(rawWiFiForm_sent);
    } else {
        WiFiForm += FPSTR(rawWiFiForm_part2);
        WiFiForm += WiFiList;
        WiFiForm += FPSTR(rawWiFiForm_part3);
    }
    return rawHeader_part1 + String(WiFiForm.length()) + rawHeader_part2 + WiFiForm;
}
