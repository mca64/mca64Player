# mca64Player
<img width="1282" height="1036" alt="Bez tytułu" src="https://github.com/user-attachments/assets/5423642e-e2ae-40ef-bb9f-5b98c8178b65" />

<img width="1282" height="1036" alt="Bez tytułu2" src="https://github.com/user-attachments/assets/5750aeef-7b29-425b-b013-017f01dcf2f0" />

Odtwarzacz plików WAV64 na Nintendo 64, oparty o libdragon.

## Funkcje

- Odtwarzanie plików WAV64 z obsługą PCM, VADPCM, Opus
- Wybór rozdzielczości ekranu (PAL/NTSC, progresywne/interlaced, profile z gier)
- HUD z informacjami o pliku, czasie, bitrate, liczbie kanałów, głośności
- Mierniki VU (poziomów audio)
- Mierniki wydajności: FPS, CPU, RAM
- System menu sterowany padem N64
- Obsługa pętli, przewijania, pauzy, regulacji głośności

## Struktura projektu

- `src/` — kod źródłowy (C, nagłówki)
- `romfs/` — pliki dołączane do obrazu ROM (np. sound.wav64)
- `Makefile` — budowanie projektu (wymaga libdragon)

## Budowanie

1. Skonfiguruj środowisko libdragon (`N64_INST` musi być ustawione)
2. Umieść plik WAV64 w `romfs/sound.wav64`
3. Uruchom:

   ```sh
   make
   ```

   Wynikowy ROM: `mca64Player.z64`

## Sterowanie (N64 pad)

- **A** — pauza/wznowienie
- **B** — stop
- **START** — menu rozdzielczości
- **L/R/D-Pad/C-Buttons** — przewijanie, regulacja głośności
- **Z** — włącz/wyłącz pętlę

## Wymagania

- libdragon (https://github.com/DragonMinded/libdragon)
- Kompilator mips64 (np. toolchain N64)

## Licencja

Projekt edukacyjny, open-source.
