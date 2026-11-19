# TriVerse Chorus

Plugin chorus stereo tiga-voice (VST3 / AU / Standalone) bergaya unit chorus rak
tiga-fase era 80-an. Dibangun dengan JUCE 8 / C++17. Konsep lengkap ada di
[konsep-teknis-tri-stereo-chorus.md](konsep-teknis-tri-stereo-chorus.md).

## Prasyarat (macOS)

1. **Xcode** (dari App Store), lalu sekali jalankan:
   ```sh
   sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
   sudo xcodebuild -license accept
   ```
2. **CMake ≥ 3.22**: `brew install cmake`

JUCE diunduh otomatis oleh CMake (tag 8.0.15). Untuk memakai salinan lokal:
`-DJUCE_DIR=/path/ke/JUCE`.

## Build

```sh
cmake -B build -G Xcode
cmake --build build --config Release
ctest --test-dir build -C Release        # unit test DSP
```

Atau tanpa Xcode IDE: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j`.

Hasil build otomatis disalin ke `~/Library/Audio/Plug-Ins/{VST3,Components}`
(matikan dengan `-DTRICHORUS_COPY_PLUGIN=OFF`). Aplikasi Standalone ada di
`build/TriVerseChorus_artefacts/Release/Standalone/`.

Universal binary (Intel + Apple Silicon): tambahkan
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

## Validasi

```sh
auval -v aufx Tvc3 Tvrs                  # validasi Audio Unit
pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/TriVerse\ Chorus.vst3
```

## Rilis otomatis (GitHub Actions)

Workflow di [.github/workflows/build.yml](.github/workflows/build.yml) membangun plugin di
macOS (universal arm64 + x86_64) dan Windows (x64), menjalankan unit test, lalu membuat
installer:

- macOS (VST3 + AU + Standalone) — [tools/macos/make-pkg.sh](tools/macos/make-pkg.sh):
  - `TriVerse-Chorus-<versi>-macOS-universal.pkg` — semua Mac, macOS 10.13+
  - `TriVerse-Chorus-<versi>-macOS-arm64.pkg` — Apple Silicon (M1 dst.), macOS 11+
  - `TriVerse-Chorus-<versi>-macOS-x86_64.pkg` — Mac Intel, macOS 10.13+
- Windows: `TriVerse-Chorus-<versi>-Windows-Setup.exe` (VST3 + Standalone) — [tools/windows/installer.iss](tools/windows/installer.iss)

Hasil build bisa diunduh dari tab **Actions → run terbaru → Artifacts**. Untuk membuat
GitHub Release dengan kedua installer terlampir, beri tag versi:

```sh
git tag v0.1.0 && git push origin v0.1.0
```

Versi diambil dari `project (... VERSION x.y.z)` di `CMakeLists.txt`. Installer belum
ditandatangani (macOS menampilkan peringatan Gatekeeper, Windows menampilkan SmartScreen).

Mencoba pembuatan `.pkg` secara lokal: `tools/macos/make-pkg.sh 0.1.0 build x86_64` → `dist/`.

## Snapshot GUI

Render panel ke PNG tanpa DAW (berguna untuk review tampilan):

```sh
cmake -B build -DTRICHORUS_BUILD_TOOLS=ON
cmake --build build --target trichorus_snapshot
./build/trichorus_snapshot_artefacts/Release/trichorus_snapshot panel.png 2
```

## Struktur

```
Source/
├── PluginProcessor.*      AudioProcessor, APVTS, bypass crossfade, peak meter
├── PluginEditor.*         Panel rak 1U (kanvas 1320×200, bisa diskalakan)
├── gui/                   LookAndFeel knob, tombol, LED, meter
├── params/                Definisi parameter
└── dsp/                   Engine DSP murni (tanpa JUCE)
tests/DspUnitTests.cpp     Tes numerik DSP
```

## Lisensi

AGPLv3 — lihat [LICENSE](LICENSE). VST adalah merek dagang Steinberg Media Technologies GmbH.
