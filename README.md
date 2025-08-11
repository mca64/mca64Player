# mca64Player

<img width="1259" height="963" alt="Bez tytułu" src="https://github.com/user-attachments/assets/29539583-e581-4809-acc9-aa98edd2b901" />


# mca64Player
Audio player for Nintendo 64


🇵🇱 mca64Player — Odtwarzacz WAV64 dla Nintendo 64

Opis:  
mca64Player to prosty odtwarzacz plików audio .wav64 dla konsoli Nintendo 64, wykorzystujący bibliotekę libdragon. Obsługuje kompresję Opus, VADPCM oraz PCM, wyświetla informacje o utworze, pasek postępu i wizualizację amplitudy (VU meter).

🔧 Wymagania
- Konsola Nintendo 64 lub emulator wspierający libdragon
- Kompilator zgodny z libdragon
- Plik audio sound.wav64 umieszczony w katalogu rom:/

🚀 Uruchamianie
1. Skompiluj projekt przy użyciu toolchaina libdragon.
2. Umieść plik sound.wav64 w katalogu ROM.
3. Uruchom program na emulatorze lub sprzęcie.

📦 Funkcje
- Inicjalizacja wyświetlania, audio, miksera i systemu plików
- Odtwarzanie pliku .wav64 z pętlą
- Wyświetlanie:
  - Nazwy pliku
  - Czasu odtwarzania i całkowitego czasu
  - Parametrów audio: częstotliwość, bitrate, kanały, bity
  - Typu kompresji
  - Liczby próbek
  - VU meter (wizualizacja głośności)

🧠 Kompresja
Obsługiwane poziomy kompresji:
- 0 — PCM (brak kompresji)
- 1 — VADPCM (domyślny)
- 3 — Opus

---

🇬🇧 mca64Player — WAV64 Audio Player for Nintendo 64

Description:  
mca64Player is a lightweight .wav64 audio player for the Nintendo 64 console, built using the libdragon library. It supports Opus, VADPCM, and PCM compression, displays track info, progress bar, and a VU meter.

🔧 Requirements
- Nintendo 64 console or emulator with libdragon support
- libdragon-compatible compiler
- Audio file sound.wav64 placed in rom:/ directory

🚀 How to Run
1. Compile the project using the libdragon toolchain.
2. Place sound.wav64 in the ROM directory.
3. Run the program on emulator or hardware.

📦 Features
- Initialization of display, audio, mixer, and file system
- Loop playback of .wav64 file
- On-screen display:
  - Filename
  - Playback time and total duration
  - Audio parameters: frequency, bitrate, channels, bits
  - Compression type
  - Sample count
  - VU meter (volume visualization)

🧠 Compression
Supported compression levels:
- 0 — PCM (no compression)
- 1 — VADPCM (default)
- 3 — Opus

---

Chcesz, żebym dodał jeszcze instrukcje kompilacji lub przykładowy plik .wav64?
