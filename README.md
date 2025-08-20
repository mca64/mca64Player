# mca64Player (NowyN)

Odtwarzacz plik�w WAV64 na Nintendo 64, oparty o libdragon.

## Funkcje

- Odtwarzanie plik�w WAV64 z obs�ug� PCM, VADPCM, Opus
- Wyb�r rozdzielczo�ci ekranu (PAL/NTSC, progresywne/interlaced, profile z gier)
- HUD z informacjami o pliku, czasie, bitrate, liczbie kana��w, g�o�no�ci
- Mierniki VU (poziom�w audio)
- Mierniki wydajno�ci: FPS, CPU, RAM
- System menu sterowany padem N64
- Obs�uga p�tli, przewijania, pauzy, regulacji g�o�no�ci

## Struktura projektu

- `src/` � kod �r�d�owy (C, nag��wki)
- `romfs/` � pliki do��czane do obrazu ROM (np. sound.wav64)
- `Makefile` � budowanie projektu (wymaga libdragon)

## Budowanie

1. Skonfiguruj �rodowisko libdragon (`N64_INST` musi by� ustawione)
2. Umie�� plik WAV64 w `romfs/sound.wav64`
3. Uruchom:

   ```sh
   make
   ```

   Wynikowy ROM: `mca64Player.z64`

## Sterowanie (N64 pad)

- **A** � pauza/wznowienie
- **B** � stop
- **START** � menu rozdzielczo�ci
- **L/R/D-Pad/C-Buttons** � przewijanie, regulacja g�o�no�ci
- **Z** � w��cz/wy��cz p�tl�

## Wymagania

- libdragon (https://github.com/DragonMinded/libdragon)
- Kompilator mips64 (np. toolchain N64)

## Licencja

Projekt edukacyjny, open-source.