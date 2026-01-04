/*
    [ MK75-Watch HTMLs ] by K-Nana
*/

#pragma once
#include <pgmspace.h>
#include <Arduino.h>

const String Error404 PROGMEM = "HTTP/1.1 404 Not Found\r\n";

const String rawHeader_part1 PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
const String rawHeader_part2 PROGMEM = "\r\n\r\n";
const String rawWiFiForm_part1 PROGMEM = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><style>body {background:linear-gradient(90deg, rgba(48, 158, 201, 1) 0%, rgba(126, 45, 196, 1) 100%);}.container {width:500px;margin:10px auto;padding:10px 30px 30px;background:#FFF;border-radius:10px;box-shadow:rgba(34, 34, 34, 0.5) 0px 0px 15px;}.textbox {width:492px;border-radius:3px;border:2px solid #ccc;}.dropdown {width:500px;border-radius:3px;border:2px solid #ccc;}.button {width:100px;height:30px;border-radius:3px;border:2px solid #2d98ca;background:#68cffe;box-shadow:rgba(34, 34, 34, 0.5) 1px 1px 2px;}.button:active {background:#60c1ee;box-shadow:inset rgba(34, 34, 34, 0.5) 1px 1px 2px;}</style><title>MK75-Watch Setup</title></head><body><div class=\"container\"><h1>Wi-Fi Setup</h1><p style=\"color:";
const String rawWiFiForm_part2 PROGMEM = "\">";
const String rawWiFiForm_part3 PROGMEM = "</p>";
const String rawWiFiForm_part4 PROGMEM = "<p>The MK75-Watch uses the Internet to synchronize its time. Enter the SSID and password in the form below and press the connect button. The SSID and password you enter here will be saved on the MK75-Watch until you delete them.</p><p>SSID<br><select id=\"ssid\" class=\"dropdown\">";
const String rawWiFiForm_part5 PROGMEM = "</select></p><p>Password<br><input type=\"password\" id=\"pass\" class=\"textbox\"></p><button class=\"button\" type=\"button\" onclick=\"connect();\">Connect</button><p>The MK75-Watch can register more than one Wi-Fi network, but the initial setup only allows one network to be registered.</p></div><script>function connect(){const ssid = document.getElementById('ssid').value;const pass = document.getElementById('pass').value;location.href=\"/wifi-setup/\"+ssid+\"/\"+pass;}</script>";
const String rawWiFiForm_part6 PROGMEM = "</body></html>";

String WiFiSetForm(bool sent, String WiFiList) {
    String WiFiForm = rawWiFiForm_part1;
    if (sent) {
        WiFiForm += "#00bb00";
        WiFiForm += rawWiFiForm_part2;
        WiFiForm += "Your request has been sent. If an error occurred, you will need to try again. Please check the screen of your MK75-Watch.";
        WiFiForm += rawWiFiForm_part3;
        WiFiForm += rawWiFiForm_part6;
    } else {
        WiFiForm += "#000000";
        WiFiForm += rawWiFiForm_part2;
        WiFiForm += rawWiFiForm_part3;
        WiFiForm += rawWiFiForm_part4;
        WiFiForm += WiFiList;
        WiFiForm += rawWiFiForm_part5;
        WiFiForm += rawWiFiForm_part6;
    }
    return rawHeader_part1+WiFiForm.length()+rawHeader_part2+WiFiForm;
}