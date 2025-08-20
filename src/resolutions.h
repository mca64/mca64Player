#ifndef RESOLUTIONS_H
#define RESOLUTIONS_H

#include <stdbool.h>
#include <display.h>
// Progressive modes
static const resolution_t PAL_640x288p      = { 640, 288, false };
static const resolution_t NTSC_640x240p     = { 640, 240, false };
static const resolution_t PAL_320x288p      = { 320, 288, false };
static const resolution_t NTSC_320x240p     = { 320, 240, false };

// Interlaced modes
static const resolution_t PAL_640x576i      = { 640, 576, true };
static const resolution_t NTSC_640x480i     = { 640, 480, true };
static const resolution_t PAL_320x576i      = { 320, 576, true };
static const resolution_t NTSC_320x480i     = { 320, 480, true };

/* 1. All-Star Baseball 2001
 * - ~512×240i: tryb hi-res (interlaced) dostępny z Expansion Pak; używane w gameplay/replay.
 *   (wartość raportowana przez listy ExpPak; dokładna wewnętrzna może się różnić)
 */
static const resolution_t PAL_AllStarBaseball2001_512x240i = { 512, 240, true };

/* 2. Armorines: Project S.W.A.R.M.
 * - 480×360i: główny hi-res interlaced w gameplay (ExpPak)
 * - 480×232i: letterbox interlaced (wariant do wyświetlania filmów/menów)
 */
static const resolution_t PAL_ArmorinesSWARM_480x360i = { 480, 360, true };
static const resolution_t PAL_ArmorinesSWARM_480x232i = { 480, 232, true };

/* 3. Army Men: Sarge's Heroes
 * - 640×240i: hi-res interlaced (wymaga Expansion Pak / opcja w menu)
 */
static const resolution_t PAL_ArmyMenSargesHeroes_640x240i = { 640, 240, true };

/* 4. Banjo-Kazooie
 * - 320×288p: standardowy progressive (PAL)
 * - uwaga: częste zmienne wewnętrzne rozdzielczości w cutscenkach
 */
static const resolution_t PAL_BanjoKazooie_320x288p = { 320, 288, false };

/* 5. Banjo-Tooie
 * - 320×288p: standardowy progressive (PAL) w gameplay; gra rzadko używała hi-res
 */
static const resolution_t PAL_BanjoTooie_320x288p = { 320, 288, false };

/* 6. Castlevania: Legacy of Darkness
 * - 490×355i: hi-res interlaced (ExpPak); tryb dostępny w opcjach, zauważalny spadek wydajności
 */
static const resolution_t PAL_CastlevaniaLoD_490x355i = { 490, 355, true };

/* 7. Donald Duck: Goin' Quackers
 * - 480×360i: hi-res interlaced (ExpPak) dla gameplay; menu zwykle w niższej rozdzielczości
 */
static const resolution_t PAL_DonaldDuckGoinQuackers_480x360i = { 480, 360, true };

/* 8. Excitebike 64
 * - 640×480i: interlaced hi-res (ExpPak) – gameplay; wymaga pamięci dla tekstur
 */
static const resolution_t PAL_Excitebike64_640x480i = { 640, 480, true };

/* 9. F-Zero X
 * - 320×288p: standardowy progressive (PAL) – gameplay
 * - uwaga: ExpPak używany głównie do edytora i niekoniecznie do większej rozdzielczości gameplay
 */
static const resolution_t PAL_FZeroX_320x288p = { 320, 288, false };

/* 10. FIFA 99
 * - 640×480i: ukryty „Super High” interlaced (raporty społeczności); nie zawsze domyślnie dostępny
 */
static const resolution_t PAL_FIFA99_640x480i = { 640, 480, true };

/* 11. GoldenEye 007
 * - 320×288p: standardowy progressive (PAL) – gameplay i menu
 * - uwaga: oryginalnie zoptymalizowane pod 320×240/288p w PAL
 */
static const resolution_t PAL_GoldenEye_320x288p = { 320, 288, false };

/* 12. Hybrid Heaven
 * - 320×288p: domyślny progressive
 * - 640×474i: interlaced hi-res (ExpPak) — opcja zwiększająca szczegóły kosztem wydajności
 * - uwaga: niektóre źródła podają 640×480i/640×474i w zależności od wersji PAL
 */
static const resolution_t PAL_HybridHeaven_320x288p = { 320, 288, false };
static const resolution_t PAL_HybridHeaven_640x474i = { 640, 474, true };

/* 13. Indiana Jones and the Infernal Machine
 * - 400×440i: Factor-5 styl custom interlaced – gameplay (wysoka pionowa rozdzielczość)
 * - uwaga: Factor-5 często używało niestandardowych timingów VI; wyjście faktycznie interlaced
 */
static const resolution_t PAL_IndianaJones_400x440i = { 400, 440, true };

/* 14. Jet Force Gemini
 * - 320×288p: standard progressive (PAL) – gameplay/menu
 * - uwaga: wewnętrzne przełączenia rozdzielczości w cutscenkach mogą występować
 */
static const resolution_t PAL_JetForceGemini_320x288p = { 320, 288, false };

/* 15. Mario Kart 64
 * - 320×288p: standard progressive (PAL) – gameplay
 * - uwaga: brak oficjalnych hi-res wariantów; renderowanie wewnętrzne dopasowane do 320×288
 */
static const resolution_t PAL_MarioKart64_320x288p = { 320, 288, false };

/* 16. Perfect Dark
 * - 320×288p: standardowy progressive (PAL)
 * - 448×268p: hi-res progressive (wariant z ExpPak / ustawienie w menu) — w PAL hires nie zawsze daje 640 szer.
 * - uwaga: NTSC/PAL różnice w skalowaniu; PAL hi-res może być inne niż NTSC high-res
 */
static const resolution_t PAL_PerfectDark_320x288p = { 320, 288, false };
static const resolution_t PAL_PerfectDark_448x268p = { 448, 268, false };

/* 17. Quake II
 * - 320×240p (nominalnie): gra koncentrowała się na wyższej głębi kolorów / teksturach; konkretne tryby zależne od portu
 * - uwaga: niektóre buildy mogły oferować alternatywne tryby graficzne z ExpPak
 */
static const resolution_t PAL_QuakeII_Default320x240p = { 320, 240, false };

/* 18. Rayman 2: The Great Escape
 * - 320×288p: standard gameplay (PAL)
 * - 480×360p: progressive hi-res — render wewnętrzny bliższy 480×360, ale VI mogło zmieniać wyjście
 * - uwaga: istnieją rozbieżności źródeł; oznaczono jako „wewnętrzny render ≈ 480×360, wyjście zależne od implementacji”
 */
static const resolution_t PAL_Rayman2_320x288p = { 320, 288, false };
static const resolution_t PAL_Rayman2_480x360p = { 480, 360, false };

/* 19. Re-Volt
 * - 640×240p: progresywny tryb medium/hi-res (warianty PAL); menu/options pozwalają zmieniać tryb
 * - uwaga: część buildów mogła outputować inne wartości po resamplingu
 */
static const resolution_t PAL_ReVolt_640x240p = { 640, 240, false };

/* 20. Resident Evil 2
 * - 320×288p: progressive stosowane w menu / niektórych statycznych ekranach
 * - 640×576i: interlaced stosowane w gameplay i FMV (ExpPak zwiększa detale i tekstury)
 * - uwaga: gra dynamicznie przełącza tryby w zależności od ekranu (stąd mieszany charakter)
 */
static const resolution_t PAL_RE2_320x288p = { 320, 288, false };
static const resolution_t PAL_RE2_640x576i = { 640, 576, true };

/* 21. Road Rash 64
 * - 640×240p: reported progressive hi-res / letterbox options in PAL builds
 * - uwaga: dokumentacja niejednoznaczna; traktować jako raport społeczności
 */
static const resolution_t PAL_RoadRash64_640x240p = { 640, 240, false };

/* 22. Rogue Squadron
 * - 400×440i: Factor-5 custom interlaced hi-res (gameplay)
 * - uwaga: output faktycznie interlaced, wewnętrzny rendering dostosowany do tego trybu
 */
static const resolution_t PAL_RogueSquadron_400x440i = { 400, 440, true };

/* 23. Star Wars: Episode I Racer
 * - 640×480i: interlaced hi-res (ExpPak) — gameplay
 */
static const resolution_t PAL_SWEpisodeIRacer_640x480i = { 640, 480, true };

/* 24. Top Gear Overdrive
 * - 320×288p: standard progressive PAL
 * - 640×240p: hi-res progressive (ExpPak) — opcja "HI-RES" w menu
 * - uwaga: 640×240p jest powszechnie raportowane jako tryb hi-res tej gry (nie 640×288)
 */
static const resolution_t PAL_TopGearOverdrive_320x288p = { 320, 288, false };
static const resolution_t PAL_TopGearOverdrive_640x240p = { 640, 240, false };

/* 25. Turok 2: Seeds of Evil
 * - 320×288p: base progressive mode (PAL)
 * - 480×360i: interlaced hi-res (ExpPak) — gameplay w trybie hi-res
 * - 480×232i: letterbox variant przy niektórych ustawieniach
 */
static const resolution_t PAL_Turok2_320x288p = { 320, 288, false };
static const resolution_t PAL_Turok2_480x360i = { 480, 360, true };
static const resolution_t PAL_Turok2_480x232i = { 480, 232, true };

/* 26. Turok: Rage Wars
 * - 320×288p: standard
 * - 480×360i: hi-res (ExpPak)
 */
static const resolution_t PAL_TurokRageWars_320x288p = { 320, 288, false };
static const resolution_t PAL_TurokRageWars_480x360i = { 480, 360, true };

/* 27. Vigilante 8
 * - 320×288p: default progressive
 * - 480×360i: interlaced hi-res (ExpPak)
 * - 640×480i: ukryty ultra-hi-res w kodzie (raporty społeczności)
 */
static const resolution_t PAL_Vigilante8_320x288p = { 320, 288, false };
static const resolution_t PAL_Vigilante8_480x360i = { 480, 360, true };
static const resolution_t PAL_Vigilante8_640x480i = { 640, 480, true };

/* 28. World Driver Championship
 * - 640×480i: interlaced hi-res (ExpPak) — gameplay
 */
static const resolution_t PAL_WorldDriver_640x480i = { 640, 480, true };

/* 29. The Legend of Zelda: Majora’s Mask
 * - 320×288p progressive (50 Hz) – gameplay i cutscenki
 * - Wymaga Expansion Pak, ale dodatkowa pamięć nie zwiększa rozdzielczości
 */
static const resolution_t PAL_Zelda_MM_320x288p = { 320, 288, false };

/* 30. The Legend of Zelda: Ocarina of Time
 * - 320×288p progressive (50 Hz) – cała gra
 * - Brak trybu hi-res, brak wsparcia ExpPak dla rozdzielczości
 */
static const resolution_t PAL_Zelda_OoT_320x288p = { 320, 288, false };

#endif /* RESOLUTIONS_H */
