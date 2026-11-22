# HTTP Example

This interactive example demonstrates usage of the `Gorgon::Network::HTTP` transport system, cookie management, and direct in-memory importing of images and wave audio over HTTP inside a unit-sized Gorgon UI window.

## Features Demonstrated

1. **Blocking Text Requests**: Quick synchronous fetching with `HTTP::BlockingGetText` (and when to prefer non-blocking calls in interactive UIs).
2. **Concurrent Async Image & Wave Imports (Auto-Started at Launch)**: Downloading and auto-detecting PNG/JPEG/BMP images (`ObtainBitmap`) and WAV/FLAC/OGG Vorbis audio (`ObtainMultimediaWave` / `ObtainSound`) using **two separate `HTTP` objects** (`imageHttp` and `audioHttp`) because each `HTTP` instance executes one transfer at a time.
3. **Non-Blocking GET & Response Headers**: Asynchronous text fetching via `Get()`, `ResponseHeadersReceivedEvent`, and `GetResponseHeader()` driven naturally by `window.Run()`.
4. **Request Headers**: Setting persistent global headers (`SetHeader`, `RemoveHeader`, `ClearHeaders`) and per-request overrides (`HeaderMap`).
5. **POST Requests & Binary Data**: Sending POST bodies, receiving raw bytes with `ObtainData`, and reference/file/stream variants.
6. **Cookie Management**: Automatic `Set-Cookie` parsing and persistence (`StoreCookies(true)`), inspecting attributes (`GetCookieInfo`, `GetCookies`), and manual cookies (`SetCookie`, `HTTP::Cookie::Parse`, `HTTP::Cookie::Build`, `ClearCookies`).
7. **Error Handling**: Handling HTTP status (`PageNotFound`) and media decode (`InvalidFormat`) failures via `TransferErrorEvent`.
8. **Progress Reporting & Slow Download**: Streaming a generated ~450 KB BMP slowly (~2 updates/sec) while updating both a `Widgets::Progressbar` and an `OpenRGB::Progressbar` via `MultiTargetProgressDisplay` and `SetProgressDisplay()` (plus `ProgressEvent`), inside a unit-sized fullscreen `Widgets::Panel`.

## How to Run

Build this example with `-DBUILD_EXAMPLES=ON` (and `-DHTTP=ON`, which is enabled by default):

```sh
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build --target HTTPExample
./Examples/Basics/HTTP/Bin/HTTPExample
```

On Windows, run the generated executable from `Examples/Basics/HTTP/Bin/`.
