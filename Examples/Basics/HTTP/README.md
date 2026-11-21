# HTTP Example

This example demonstrates usage of the `Gorgon::Network::HTTP` transport system, including cookie management and direct in-memory importing of images and wave audio over HTTP.

## Features Demonstrated

1. **Blocking Text Requests**: Quick synchronous fetching with `HTTP::BlockingGetText` (and when to prefer non-blocking calls).
2. **Non-Blocking GET & Response Headers**: Asynchronous text fetching via `Get()`, `ResponseHeadersReceivedEvent`, and `GetResponseHeader()`.
3. **Request Headers**: Setting persistent global headers (`SetHeader`, `RemoveHeader`, `ClearHeaders`) and per-request overrides (`HeaderMap`).
4. **POST Requests & Binary/File/Stream Transfers**: Sending POST bodies, receiving raw bytes with `ObtainData`, and reference/file/stream variants.
5. **Cookie Management**: Automatic `Set-Cookie` parsing and persistence (`StoreCookies(true)`), inspecting attributes (`GetCookie`, `GetCookieInfo`, `GetCookies`), and manual cookies (`SetCookie`, `HTTP::Cookie::Parse`, `HTTP::Cookie::Build`).
6. **Error Handling**: Handling network, HTTP status (`PageNotFound`), and decoding (`InvalidFormat`) errors via `TransferErrorEvent`.
7. **Importing Images & Bitmaps from the Internet**: Downloading and decoding PNG, JPEG, and BMP images directly into `Graphics::Bitmap` (`ObtainBitmap`) or `Containers::Image` (`ObtainImage`) with optional automatic main-thread `Prepare()`.
8. **Importing Wave Audio from the Internet**: Downloading and auto-detecting WAV, FLAC, and OGG Vorbis audio directly into `Multimedia::Wave` (`ObtainMultimediaWave` / `ObtainSound`) or `Containers::Wave` (`ObtainWave`) and playing it through `Gorgon::Audio::BasicController`.

## How to Run

Build this example with `-DBUILD_EXAMPLES=ON` (and `-DHTTP=ON`, which is enabled by default):

```sh
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build --target HTTPExample
./Examples/Basics/HTTP/Bin/HTTPExample
```

On Windows, run the generated executable from `Examples/Basics/HTTP/Bin/`.
