# PIICA 発光テストプログラム

RC-S620/S FeliCaカードリーダーを使って、PIICAのLED点灯をテストするプログラムです。

## 使用機材

* RC-S620/S FeliCaカードリーダー（dp16, dp15に接続）
* Switch Science mbed LPC824
* Seeed Grove rotary angle sensor v1.2 （dp2に接続）

## 接続図

<img src="./piica_test.JPG" width="640px">

## ビルド方法

### ビルドツールのインストール

https://os.mbed.com/docs/mbed-os/v5.15/tools/index.html

* GNU Arm Embedded version 9 (9-2019-q4-major) のインストール  
https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

* Mbed CLIのインストール

```
$ pip install mbed-cli
```

### プログラムのビルドとターゲットへの書き込み

```
$ git clone -b piica_test https://github.com/toyowata/rcs620s-test
$ cd rcs620s-test
$ mbed deploy
$ mbed compile -m ssci824 -t gcc_arm --flash
```