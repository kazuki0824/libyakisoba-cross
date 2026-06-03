# libyakisoba.so: ECM/EMM デコーダライブラリ

OishiiSlurper氏のSlurper-toolを基にライブラリ化。  
BCASカードやPC/SCサービスを用いることなく、本ライブラリ単体・独自APIで、  
ECMのbodyを受け取り復号しスクランブルキーK\_sを返す。  
また設定によりEMMからワークキーK\_wを取り出すことも可能。

## ビルド

CMake 3.16 以降が必要です。

```sh
git clone https://github.com/kazuki0824/libyakisoba-cross.git
cd libyakisoba-cross
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build
```

共有ライブラリを無効化する場合は `-DBUILD_SHARED_LIBS=OFF` を指定します。

```sh
cmake -S . -B build -DBUILD_SHARED_LIBS=OFF
cmake --build build
```

### Android NDK

Android NDK の CMake toolchain を指定してビルドします。

```sh
cmake -S . -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-23 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-android
```

### WebAssembly / Emscripten

```sh
emcmake cmake -S . -B build-wasm -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
ctest --test-dir build-wasm --output-on-failure
```

## 使用方法

- クライアントプログラムのソースで `#include <yakisoba.h>`する。
- `bcas_decodeECM()` や `bcas_decodeEMM()` をコールする。(yakisoba.h を参照)
- `yakisoba::yakisoba` CMake target、または `-lyakisoba` でリンクする。
- 実行前にあらかじめキーを記述した設定ファイルを用意しておく。

## 設定ファイル

ワークキーの値を記した設定ファイルをロード時に読み込んで使用するので、
下記のいずれかの場所に用意する必要がある。
1. 環境変数BCAS\_KEYS\_FILE に指定したパス
1. `~/.bcas_keys`
1. `$(sysconfdir)/bcas_keys`
上記の順に探されて、最初に見つかったファイルが使用される。  
フォーマットは付属のbcas\_keysを参照のこと。

EMMのデコードを行う場合は、
カードID及びマスターキーK\_mの値も、上記設定ファイルに記述する必要がある。
