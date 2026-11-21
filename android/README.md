# OpenNetview Android client

This Android app is a native counterpart to the SDL desktop client. It connects to the camera's TCP command port, receives RTP/H.264 on UDP, decodes video with Android `MediaCodec`, and provides touch controls and a Home command.

## Build

Open this `android/` directory in Android Studio (Android Gradle Plugin 8.7.3, Kotlin 2.0.21, JDK 17), or build with an installed Gradle 8.9+ and Android SDK:

```sh
gradle assembleDebug
```

The debug APK is written to `app/build/outputs/apk/debug/app-debug.apk`. This repository contains the Android source and build configuration; it does not check in a generated APK or Gradle wrapper.

## Use

Install the debug APK on an Android device on the same network as the camera. Enter its IP address or hostname and tap **Connect**. The app binds UDP port 6969 for RTP/H.264 and connects to TCP port 52000 for control. Hold the directional buttons to send repeated movement commands; **Home** sends `goto 0 0`.

The camera firmware must support the same command framing and RTP payload format as the desktop client. Android devices and network configurations may impose additional constraints on UDP reception.
