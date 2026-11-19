# Dokumen Konsep Teknis — Plugin Tri-Stereo Chorus

**Nama kerja:** TriVerse Chorus (nama sementara, bukan nama merek pihak lain)
**Versi dokumen:** 0.1 — Konsep
**Target format:** VST3, AU, (opsional) AAX
**Framework:** JUCE 8 / C++17

---

## 1. Ringkasan

Plugin ini adalah efek chorus stereo tiga-suara (*three-voice*) yang terinspirasi oleh unit chorus rak analog era 1980-an yang dikenal lewat suara electric piano yang lebar dan "berputar". Ciri utamanya:

- Tiga jalur delay termodulasi yang berjalan paralel.
- Tiga LFO dengan selisih fase tetap (default 120°), sehingga modulasi tidak pernah sinkron.
- Tiap jalur ditempatkan di posisi stereo yang berbeda (kiri, tengah, kanan), menghasilkan gerakan spasial, bukan sekadar pelebaran.
- Karakter analog dari emulasi chip BBD (bucket-brigade delay) beserta filter dan compander di sekitarnya.

Tujuan produk bukan menyalin satu unit secara identik, melainkan menangkap **perilaku** dan **karakter** sirkuit sejenis dengan identitas produk sendiri.

---

## 2. Tujuan & Batasan

### Tujuan
1. Suara chorus tiga-fase yang lebar, halus, dan musikal di electric piano, gitar clean, pad, dan vokal.
2. Karakter analog yang meyakinkan: sedikit gelap, sedikit noise, modulasi yang "hidup".
3. Latensi nol (tanpa lookahead).
4. Beban CPU rendah: target < 1% per instance pada CPU desktop modern di 48 kHz.
5. Tanpa alias yang terdengar pada setting ekstrem.

### Batasan / Non-tujuan (v1)
- Tidak memodelkan setiap komponen sirkuit (bukan simulasi SPICE real-time).
- Tidak ada efek flanger/vibrato mode di v1 (bisa jadi roadmap).
- Tidak ada sinkronisasi tempo LFO di v1 (roadmap).

---

## 3. Arsitektur Sinyal

```
                     ┌──────────────────────────────────────────────────────────┐
 In L ──┐            │   JALUR WET (diulang 3×, satu per voice)                 │
        ├─► Input ───┼─► Compressor ─► AA LPF ─► BBD Delay ─► Recon LPF ─► Expander ─┼─► Pan Matrix ─┐
 In R ──┘   Stage    │   (NE570-ish)    (≈8–10k)   (modulasi     (≈8–10k)   (NE570-ish)│               │
            (mono    │                             LFO n)                             │               │
             sum /   └──────────────────────────────────────────────────────────┘               │
             stereo)                                                                             ▼
                                                                                   ┌────────────────────┐
 Dry L/R ───────────────────────────────────────────────────────────────────────►  │  Mix Dry/Wet +     │─► Out L
                                                                                   │  Width + Output    │─► Out R
                                                                                   └────────────────────┘

 LFO Bank:  LFO1 (φ = 0°)   LFO2 (φ = 120°)   LFO3 (φ = 240°)
            + drift acak lambat per LFO (opsional)
```

### Catatan desain
- **Compressor dan expander dibagi per voice atau dipakai bersama?** Pada banyak desain hardware, compander dipakai di sebelum dan sesudah seluruh blok BBD. Untuk efisiensi, v1 memakai **satu compressor bersama** di depan dan **satu expander per voice** di belakang (karena tiap voice punya delay berbeda). Perlu divalidasi terhadap referensi.
- **Input mono vs stereo.** Default: input di-*sum* ke mono sebelum jalur wet (perilaku khas unit chorus rak), jalur dry tetap stereo.

---

## 4. Rincian Modul DSP

### 4.1 Input Stage
- Gain input (−12 dB s/d +12 dB).
- Soft clipper ringan (tanh atau polinomial kubik) untuk meniru headroom op-amp. Default hampir transparan; baru terasa jika input didorong.
- DC blocker (HPF satu-pole ~10 Hz).

### 4.2 Compressor (Pra-BBD)
Meniru compander 2:1 yang umum dipakai untuk menekan noise BBD.

- **Detektor:** rectifier rata-rata (bukan true RMS) dengan filter satu-pole.
  - Attack/release efektif ditentukan satu konstanta waktu (awal: ~10–20 ms, tuning dari referensi).
- **Gain:** `g = 1 / sqrt(env / ref)`, dibatasi agar tidak meledak di sinyal sangat kecil.
- Hasil samping yang diinginkan: sedikit "pumping" dan transien yang melunak pada sinyal wet.

### 4.3 Anti-Aliasing Filter (AA LPF)
- Low-pass 2 s/d 4 kutub (dua biquad Butterworth atau model Sallen-Key), cutoff ~8–10 kHz.
- Di hardware, filter ini ada karena BBD adalah sistem sampel diskrit. Di plugin, filter ini justru pembentuk karakter utama (suara "gelap").

### 4.4 Model BBD Delay
Ini inti karakter. Diusulkan dua tingkat implementasi:

#### Level A — Modulated Delay + Filter (v1, direkomendasikan untuk mulai)
- Delay line sirkular dengan **interpolasi fraksional** (Hermite 4-titik atau Lagrange orde-3; linear terlalu "kusam" dan menimbulkan modulasi amplitudo).
- Waktu delay mengikuti rumus BBD:

  ```
  t_delay = N_stages / (2 × f_clock)
  ```
  Contoh: 1024 tahap, clock 100 kHz → 5,12 ms.

  Modulasi dilakukan pada **clock**, bukan langsung pada waktu delay. Karena hubungannya berbanding terbalik, modulasi clock yang simetris menghasilkan modulasi waktu delay yang sedikit asimetris, dan ini ikut membentuk karakter.

- Tambahan karakter:
  - Saturasi ringan di dalam jalur (asimetris kecil untuk harmonik genap).
  - Noise lantai BBD (white noise −80 s/d −70 dBFS, difilter), dilewatkan compander sehingga ikut "bernapas".
  - Kehilangan sinyal kecil per tahap (redaman HF tambahan yang bergantung panjang delay).

#### Level B — Emulasi BBD Berbasis Clock (v2 / mode "Vintage")
- Simulasi proses sample-and-hold pada laju clock BBD sesungguhnya, termasuk aliasing dan *imaging* yang difilter oleh model filter input/output.
- Referensi pendekatan: Holters & Parker (DAFx-18), yang menggabungkan BBD dan filter input/output-nya dalam satu model efisien.
- Lebih akurat, tetapi lebih mahal CPU dan lebih rumit. Dijadwalkan setelah Level A matang.

### 4.5 LFO Bank
- 3 LFO bersama satu fase master, dengan offset 0°, 120°, 240°.
- Bentuk gelombang: sinus (default) dan segitiga. Opsi "analog" = segitiga yang sedikit dibulatkan (lewat LPF atau waveshaper) seperti LFO integrator op-amp.
- **Rate:** 0,05–10 Hz (skala logaritmik).
- **Depth:** 0–100%, dipetakan ke rentang deviasi clock.
- **Drift:** noise acak sangat lambat (< 0,2 Hz) per LFO, amplitudo kecil, agar tiga LFO tidak terdengar terlalu "digital sempurna".
- **Spread fase:** parameter tersembunyi/lanjutan (0–180°) untuk eksperimen; default 120°.

### 4.6 Reconstruction Filter (Recon LPF)
- Serupa AA LPF (cutoff ~8–10 kHz). Pada hardware, filter ini meredam komponen clock dan *image*.
- Level A: cukup biquad. Level B: bagian dari model gabungan.

### 4.7 Expander (Pasca-BBD)
- Kebalikan compressor: rasio 1:2, `g = env / ref` dengan detektor yang sama sifatnya.
- Ketidakcocokan kecil antara compressor dan expander (misal perbedaan konstanta waktu) disengaja karena menghasilkan artefak "napas" yang khas.

### 4.8 Clock Bleed (opsional)
- Tonal sangat halus di frekuensi clock yang ter-*alias* ke rentang terdengar. Default **off** atau sangat pelan; bisa dimasukkan ke mode "Vintage".

### 4.9 Pan Matrix, Width & Mix
- Posisi default voice: V1 = kiri penuh, V2 = tengah, V3 = kanan penuh.
- **Width (0–100%):** interpolasi posisi pan dari semua-tengah (mono) ke posisi default.
- Hukum pan: constant-power (−3 dB di tengah).
- **Mix:** dry/wet equal-power, atau mode alternatif "wet-only" untuk dipakai di bus aux.
- **Output gain:** −12 dB s/d +12 dB.

---

## 5. Daftar Parameter (v1)

| ID | Nama | Rentang | Default | Skala | Catatan |
|---|---|---|---|---|---|
| `input` | Input | −12 … +12 dB | 0 dB | linear dB | mendorong soft clip |
| `rate` | Rate | 0,05 … 10 Hz | 0,6 Hz | log | laju LFO |
| `int1`–`int3` | Intensity I / II / III | 0 … 100 % | 45 % | linear | deviasi clock per voice (100 % = ±30 % clock) |
| `delay` | Delay | 2 … 20 ms | 7 ms | log | waktu delay tengah |
| `shape` | Shape | Sine / Tri / Analog | Sine | pilihan | bentuk LFO |
| `width` | Width | 0 … 100 % | 100 % | linear | sebaran stereo |
| `tone` | Tone | 4 … 14 kHz | 9 kHz | log | cutoff AA/recon |
| `char` | Character | 0 … 100 % | 30 % | linear | saturasi, noise, drift sekaligus |
| `mix` | Mix | 0 … 100 % | 50 % | linear | dry/wet |
| `output` | Output | −12 … +12 dB | 0 dB | linear dB | |
| `bypass` | Bypass | on/off | off | — | bypass tanpa klik |

Semua parameter kontinu memakai **smoothing** (±20 ms) untuk mencegah zipper noise.

---

## 6. Pseudocode Inti (Level A)

```cpp
// per blok
for (int n = 0; n < numSamples; ++n)
{
    float x   = monoSum(inL[n], inR[n]) * inputGain;
    x         = softClip(dcBlock(x));
    float xc  = compressor.process(x);          // bersama
    xc        = aaFilter.process(xc);
    delayLine.write(xc + bbdNoise.next());

    float outL = 0.f, outR = 0.f;
    for (int v = 0; v < 3; ++v)
    {
        float lfo    = lfoBank.value(v);               // −1 … +1, sudah termasuk drift
        float clock  = baseClock * (1.f + depth * lfo);
        float tDelay = N_STAGES / (2.f * clock);       // detik
        float y      = delayLine.readHermite(tDelay * sampleRate);
        y            = bbdSaturate(y, character);
        y            = reconFilter[v].process(y);
        y            = expander[v].process(y);
        outL += y * panL[v];
        outR += y * panR[v];
    }
    lfoBank.advance();

    outBufL[n] = mixEqualPower(inL[n], outL, mix) * outputGain;
    outBufR[n] = mixEqualPower(inR[n], outR, mix) * outputGain;
}
```

`baseClock` diturunkan dari parameter `delay` (dibalik dari rumus BBD), sehingga pengguna mengatur milidetik, sementara modulasi tetap bekerja di domain clock.

---

## 7. Pertimbangan Kualitas & Performa

- **Oversampling:** Level A umumnya tidak butuh, karena nonlinearitasnya ringan. Sediakan opsi 2× untuk saturasi jika `char` tinggi. Level B kemungkinan membutuhkan pendekatan multirate internal.
- **Denormal:** aktifkan `juce::ScopedNoDenormals` dan tambah DC/noise sangat kecil di filter rekursif.
- **Laju sampel:** semua koefisien filter dan konstanta waktu dihitung ulang di `prepareToPlay` untuk 44,1–192 kHz.
- **Ukuran buffer delay:** alokasikan untuk delay maksimum + deviasi maksimum + margin interpolasi, pada laju sampel tertinggi.
- **Determinisme:** generator noise/drift memakai seed tetap per instance agar render offline konsisten.
- **Mono compatibility:** uji penjumlahan L+R; chorus tiga-fase bisa menghasilkan cancel parsial. Pertimbangkan sedikit perbedaan filter antar-voice untuk mengurangi comb yang tajam saat mono.

---

## 8. Struktur Kode (JUCE)

```
Source/
├── PluginProcessor.{h,cpp}     // AudioProcessor, APVTS, routing
├── PluginEditor.{h,cpp}        // GUI
├── dsp/
│   ├── TriChorusEngine.{h,cpp} // merangkai semua modul
│   ├── Compander.h             // Compressor & Expander
│   ├── BbdDelayLine.h          // buffer + interpolasi Hermite
│   ├── BbdFilter.h             // AA & recon LPF
│   ├── LfoBank.h               // 3 LFO + drift
│   ├── Saturation.h
│   └── PanMatrix.h
└── params/
    └── ParameterLayout.h       // definisi & rentang parameter
tests/
├── DspUnitTests.cpp            // tes numerik per modul
└── RenderTests.cpp             // render file referensi untuk A/B
```

Prinsip: kelas DSP tidak bergantung pada JUCE GUI sehingga bisa diuji terpisah (dan idealnya diprototipe dulu di Python dengan struktur yang sama).

---

## 9. Validasi & Pengujian

### 9.1 Jika ada akses ke hardware referensi
1. **Sine sweep** kering vs basah → respons frekuensi jalur wet (menentukan cutoff dan kemiringan filter).
2. **Sinus stabil 1 kHz** → ekstrak modulasi fase/frekuensi dari output untuk mengukur bentuk, laju, dan kedalaman LFO serta rentang delay.
3. **Burst dan tone level bertahap** → ukur perilaku compander (konstanta waktu, kurva gain).
4. **Input diam** → spektrum noise dan clock bleed.
5. **Input level tinggi** → kurva saturasi dan harmonik.

### 9.2 Tanpa hardware
- Gunakan rekaman referensi yang sah (misalnya sampel dari library yang dimiliki) dan lakukan perbandingan telinga serta spektrogram.
- Tuning dilakukan per parameter dengan preset "target".

### 9.3 Tes teknis
- Unit test: respons filter, akurasi delay, kontinuitas LFO, gain compander.
- Stres test: otomasi cepat semua parameter, perubahan buffer size, perubahan sample rate.
- Kompatibilitas: pluginval (tingkat strictness tinggi), Reaper, Ableton Live, Logic Pro, FL Studio, Cubase, Pro Tools (jika AAX).

---

## 10. Rencana Kerja (Milestone)

| Tahap | Isi | Keluaran |
|---|---|---|
| M0 | Prototipe Python: chorus 3-voice + filter + compander | file audio A/B |
| M1 | Port ke JUCE, parameter dasar, GUI generik | plugin berfungsi (VST3/AU) |
| M2 | Karakter: saturasi, noise, drift, tuning filter | build "beta sound" |
| M3 | GUI kustom, preset, bypass tanpa klik | build fitur lengkap |
| M4 | Validasi (pluginval, lintas DAW), optimasi CPU | release candidate |
| M5 (opsional) | Mode Vintage (Level B BBD), tempo sync, mode vibrato | v1.1 / v2 |

---

## 11. Identitas Produk & Aspek Legal

- Gunakan **nama, logo, dan desain GUI sendiri**. Jangan memakai nama merek hardware maupun merek plugin lain di produk.
- Deskripsi pemasaran sebaiknya bersifat umum, misalnya "terinspirasi chorus rak tiga-fase klasik 80-an".
- Jika memakai JUCE, perhatikan lisensinya (AGPLv3 atau lisensi komersial sesuai tingkat pendapatan).
- VST3 memerlukan persetujuan lisensi Steinberg; AAX memerlukan pendaftaran di program developer Avid.
- Ini bukan nasihat hukum; konsultasikan ke ahli jika produk akan dijual.

---

## 12. Referensi Teknis

- M. Holters & J. D. Parker, *A Combined Model for a Bucket Brigade Device and its Input and Output Filters*, DAFx-18.
- C. Raffel & J. O. Smith, *Practical Modeling of Bucket-Brigade Device Circuits*, DAFx-10.
- J. O. Smith, *Physical Audio Signal Processing* — bagian delay line dan interpolasi fraksional (CCRMA, Stanford).
- Dokumentasi JUCE: `juce::dsp`, `AudioProcessorValueTreeState`.
- Datasheet chip BBD (keluarga MN3xxx) dan compander NE570/NE571 untuk angka tahap, rentang clock, dan kurva kompresi.

---

## 13. Pertanyaan Terbuka

1. Apakah tersedia akses ke unit hardware atau rekaman referensi untuk pengukuran?
2. Format target prioritas: VST3/AU saja, atau AAX juga di v1?
3. Model bisnis: gratis, berbayar, atau open source? (memengaruhi pilihan lisensi JUCE)
4. Preferensi gaya GUI: skeuomorfik (panel rak) atau flat/modern?
