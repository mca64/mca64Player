# mca64Player

<img width="1259" height="963" alt="Bez tytułu" src="https://github.com/user-attachments/assets/29539583-e581-4809-acc9-aa98edd2b901" />


# mca64Player  
Audio player for Nintendo 64  

---

🇵🇱 **mca64Player — Odtwarzacz WAV64 dla Nintendo 64**  

**Opis:**  
mca64Player to prosty odtwarzacz plików audio `.wav64` dla konsoli Nintendo 64, wykorzystujący bibliotekę **libdragon**. Obsługuje kompresję **Opus**, **VADPCM** oraz **PCM**, wyświetla informacje o utworze, pasek postępu, wizualizację amplitudy (VU meter) oraz menu wyboru rozdzielczości ekranu.  

---

### 🔧 Wymagania  
- Konsola **Nintendo 64** lub emulator wspierający **libdragon**  
- Kompilator zgodny z **libdragon**  
- Plik audio `sound.wav64` w katalogu `rom:/`  

---

### 🚀 Uruchamianie  
1. Skompiluj projekt przy użyciu toolchaina **libdragon** (patrz niżej).  
2. Umieść plik `sound.wav64` w katalogu ROM.  
3. Uruchom program na emulatorze lub sprzęcie.  
4. Przy starcie programu pojawi się **menu wyboru rozdzielczości** — wybierz strzałkami, zatwierdź **A**, anuluj **B** lub **Start**.  

---

### 📦 Funkcje  
- Inicjalizacja wyświetlania, audio, miksera i systemu plików  
- **Menu wyboru rozdzielczości** z obsługą przewijania i powtarzania klawiszy  
- Odtwarzanie pliku `.wav64` z pętlą  
- Wyświetlanie:  
  - Nazwy pliku  
  - Czasu odtwarzania i całkowitego czasu  
  - Parametrów audio: częstotliwość, bitrate, kanały, bity  
  - Typu kompresji  
  - Liczby próbek  
  - VU meter (wizualizacja głośności)  

---

### 🧠 Kompresja  
Obsługiwane poziomy kompresji:  
- **0** — PCM (brak kompresji)  
- **1** — VADPCM (domyślny)  
- **3** — Opus  

---

### 🖥️ Kompilacja  
Zakładamy, że masz już zainstalowany toolchain **libdragon**:  

```bash
# Pobierz kod źródłowy libdragon i zainstaluj (jeśli jeszcze nie masz)
git clone https://github.com/DragonMinded/libdragon.git
cd libdragon
make
make install

# Wróć do katalogu projektu mca64Player
cd ../mca64Player

# Skompiluj ROM
make
