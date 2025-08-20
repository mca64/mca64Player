# mca64Player (NowyN)

Odtwarzacz plików WAV64 na Nintendo 64, oparty o libdragon.

## Funkcje

- Odtwarzanie plików WAV64 z obs³ug¹ PCM, VADPCM, Opus
- Wybór rozdzielczoœci ekranu (PAL/NTSC, progresywne/interlaced, profile z gier)
- HUD z informacjami o pliku, czasie, bitrate, liczbie kana³ów, g³oœnoœci
- Mierniki VU (poziomów audio)
- Mierniki wydajnoœci: FPS, CPU, RAM
- System menu sterowany padem N64
- Obs³uga pêtli, przewijania, pauzy, regulacji g³oœnoœci

## Struktura projektu

- `src/` — kod Ÿród³owy (C, nag³ówki)
- `romfs/` — pliki do³¹czane do obrazu ROM (np. sound.wav64)
- `Makefile` — budowanie projektu (wymaga libdragon)

## Budowanie

1. Skonfiguruj œrodowisko libdragon (`N64_INST` musi byæ ustawione)
2. Umieœæ plik WAV64 w `romfs/sound.wav64`
3. Uruchom:

   ```sh
   make
   ```

   Wynikowy ROM: `mca64Player.z64`

## Sterowanie (N64 pad)

- **A** — pauza/wznowienie
- **B** — stop
- **START** — menu rozdzielczoœci
- **L/R/D-Pad/C-Buttons** — przewijanie, regulacja g³oœnoœci
- **Z** — w³¹cz/wy³¹cz pêtlê

## Wymagania

- libdragon (https://github.com/DragonMinded/libdragon)
- Kompilator mips64 (np. toolchain N64)

## Licencja

Projekt edukacyjny, open-source.