# YMF825をUSB-MIDIデバイスとしてコントロールする

AVRマイコン ATmega32U4を使いLUFAライブラリのDualMIDIデバイスのサンプルを参考に  
YMF825をUSB-MIDIデバイスとしてコントロールしてみました。  
###### BankSelect=1、にするとGM音源として振る舞いますがCh10は普通にメロディとして鳴ります。

### コンパイル
    ダウンロードして展開後DebugフォルダのMakeファイルを実行すると"YMF825MIDI_CONTROL.hex"が生成されますので  
    マイコンに書き込んで下さい。

### 接続

#### ハードウエア
    ATmega32U4を使ったマイコンボード  
    ArduinoのLeonard等32U4使ったものは直接hexファイルを書きこめば動くかもしれませんが試していません
    
      PB0->SS　　
      PC6->Reset　　
      PB1->CLK　　
      PB2->MOSI　　
      PB3->MISO (プログラム上では未使用）

#### USB-MIDデバイス
    USB-MIDIデバイスとして"LUFA Dual MIDI Demo"と"MIDIOUT2 (LUFA Dual MIDI Demo)"の2つのMIDIデバイスのうち
    シーケンサソフト等のMIDI-OUTを"LUFA Dual MIDI Demo"に設定して下さい。  
    "MIDIOUT2 (LUFA Dual MIDI Demo)"の方は音色エディタのコマンド送受信用です。

## 音源部MIDIサポート(LUFA Dual MIDI Demo)

##### バンクセレクト、プログラムチェンジ　　
    BankSelect(cc0)を使ってプログラムチェンジでロードする音色データを選択（全MIDIチャンネルが変わります）

    0: YMF825の音色番号(No.1-No.16）   
    1: MA-3データで作ったGM音色(No.1-No.128)   
    2: EEPROM EEPROMからの音色読み出し(No.1-No.32)  

#### ピッチベンド、ピッチベンドセンシティブ
    ピッチベンドセンシティブ値0-24

#### その他
    Velocity
    Hold
    Expression
    Modulation
    PartLevel
    
## コントロール部MIDIコマンド"MIDIOUT2 (LUFA Dual MIDI Demo)"
音原設定等のコマンドやデータを"MIDIOUT2 (LUFA Dual MIDI Demo)"宛に  
システムエクルシーブメッセージを利用して送受信しています　　
規格に定められたヘッダ等は含んでいませんので他の機器にこのデータが流れないようにして下さい。

#### COMMANDフォーマット
    4バイトのデータを上位4bitと下位4bitを分けて10byteのメッセージとして送信しています 
    フォーマット 
    "0xf0, COMMAND_hi, COMMAND_lo, DATA１_hi, DATA1_lo, DATA2_hi, DATA2_lo, DATA3_hi, DATA3_lo, 0xf7"
#### COMMAND
    0. YMF825のリセット
    1. レジスタへ書き込み DATA1=addres,DATA2=data
    2.Playモード設定
      DATA1=0 mono_mode
      DATA1=1 Poly_mode
      DATA1=3 Poly_mode2: 指定チャンネルの＋８チャンネルを同時発音させます
    5. EEPROM　write       DATA1番地へData2を書き込み
    6. EEPROM tone read   EEPROMのDATA1番音色データを上位と下位4bitづつに分けた６２バイトのシステムエクルシーブメッセージで返す
    9. write burst          音色バッファのYMF825へ全書き込み
    10. write data and burst 音色バッファをDATA1番地へDATA2を書きこみ後書き込み
    11. write data only      音色バッファのDATA1番地へData2を書き込み
    
 ## おまけ
 /etcにJavaで作った音色エディタを添付しておきました。
