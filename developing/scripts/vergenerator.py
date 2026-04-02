import datetime
import tkinter
from tkinter import simpledialog, messagebox

def main():
  devVer = messagebox.askyesno('確認', 'スナップショットとしてコンパイルしますか？')
  today = datetime.date.today()
  year = (today.year)%100
  month = today.month
  if devVer:
    day = today.day
    minor = 0
  else:
    day = 0
    minor = simpledialog.askinteger("入力", "バージョンは"+str(year)+"."+str(month)+".xです。\nマイナーバージョンを入れてください。\nキャンセルを押すと0として扱います。", minvalue=0, maxvalue=9)
  if minor:
    print((year*100000)+(month*1000)+(day*10)+minor)
  else:
    print((year*100000)+(month*1000)+(day*10))

if __name__ == "__main__":
  main()