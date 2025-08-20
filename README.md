# mca64Player
<img width="1282" height="1036" alt="Bez tytułu" src="https://github.com/user-attachments/assets/5423642e-e2ae-40ef-bb9f-5b98c8178b65" />

<img width="1282" height="1036" alt="Bez tytułu2" src="https://github.com/user-attachments/assets/5750aeef-7b29-425b-b013-017f01dcf2f0" />

## WAV64 Player for Nintendo 64 (libdragon-based)

### Features
- Playback of WAV64 files with PCM, VADPCM, Opus support
- Screen resolution selection (PAL/NTSC, progressive/interlaced, game profiles)
- HUD with file info, time, bitrate, channel count, volume
- VU meters (audio levels)
- Performance meters: FPS, CPU, RAM
- Menu system controlled by N64 pad
- Loop, seek, pause, and volume control support

### Project Structure
- `src/` — source code (C, headers)
- `romfs/` — files included in ROM image (e.g. sound.wav64)
- `Makefile` — project build (requires libdragon)

### Building
1. Set up the libdragon environment (`N64_INST` must be set)
2. Place a WAV64 file in `romfs/sound.wav64`
3. Run:
   ```sh
   make
   ```
   Output ROM: `mca64Player.z64`

### Controls (N64 pad)
- **A** — pause/resume
- **B** — stop
- **START** — resolution menu
- **L/R/D-Pad/C-Buttons** — seek, volume control
- **Z** — toggle loop

### Requirements
- libdragon (https://github.com/DragonMinded/libdragon)
- mips64 compiler (e.g. N64 toolchain)

### License
Educational, open-source project.

---

## Odtwarzacz plików WAV64 na Nintendo 64, oparty o libdragon.

### Funkcje
- Odtwarzanie plików WAV64 z obsługą PCM, VADPCM, Opus
- Wybór rozdzielczości ekranu (PAL/NTSC, progresywne/interlaced, profile z gier)
- HUD z informacjami o pliku, czasie, bitrate, liczbie kanałów, głośności
- Mierniki VU (poziomów audio)
- Mierniki wydajności: FPS, CPU, RAM
- System menu sterowany padem N64
- Obsługa pętli, przewijania, pauzy, regulacji głośności

### Struktura projektu
- `src/` — kod źródłowy (C, nagłówki)
- `romfs/` — pliki dołączane do obrazu ROM (np. sound.wav64)
- `Makefile` — budowanie projektu (wymaga libdragon)

### Budowanie
1. Skonfiguruj środowisko libdragon (`N64_INST` musi być ustawione)
2. Umieść plik WAV64 w `romfs/sound.wav64`
3. Uruchom:
   ```sh
   make
   ```
   Wynikowy ROM: `mca64Player.z64`

### Sterowanie (N64 pad)
- **A** — pauza/wznowienie
- **B** — stop
- **START** — menu rozdzielczości
- **L/R/D-Pad/C-Buttons** — przewijanie, regulacja głośności
- **Z** — włącz/wyłącz pętlę

### Wymagania
- libdragon (https://github.com/DragonMinded/libdragon)
- Kompilator mips64 (np. toolchain N64)

### Licencja
Projekt edukacyjny, open-source.
