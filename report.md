# Code Review Report — DX_FT8_Port_Tab5

This is a pretty solid port overall. The WSJT‑X FT8 modem has been moved over to the M5Stack Tab5 (ESP32‑P4) with a lot of the expected pieces in place: FIR filtering, a 2048‑point FFT, the LDPC(174,91) decoder chain, SI5351 VFO control, I2S/SGTL5000 audio handling, the autosequencer state machine, ADIF logging, and the M5GFX touch UI.

The good news is that the DSP/codec core in `decode/pack/unpack/ldpc/Geodesy` is a faithful port of the well‑tested C code and it looks clean. The problems I found are mostly in the application glue layer rather than in the core FT8 engine itself.

**Status legend:** ✅ fixed in this review · ⚠️ noted, not changed

## Quick take

This port is in good shape. The bigger issues were the kind of things that tend to show up in embedded work: buffer sizing mistakes, stale state, a couple of unsafe string copies, and a few logic errors that looked harmless until they started behaving badly under real use. Once those were cleaned up, the project looked much more robust.
The other thing to consider is that I think the buffer sizes of 22 bytes were intended to limit the size of strings written to the display. You can't do it this way - it causes a buffer overrun. The strings should be truncated when writing to the display. I have not done this (yet!).
---

### 1. ✅ Uninitialized variable shadowing global in `Display_WF()`
- **File:** `src/Display.cpp`
- **What was going on:** A local `int xmit_flag;` was shadowing the global one, so the waterfall cursor color was reading from an uninitialized local variable. That is exactly the kind of bug that leads to odd, random behavior.
- **Fix:** The local variable was removed so the global value is used consistently.

### 2. ✅ Off‑by‑one out‑of‑bounds write into `FT8_Data[]`
- **File:** `src/SDR_Audio.cpp`
- **What was going on:** `FT8_Data` was an `int16_t[1024]`, but the wrap check used `if (ft8_index > FT8_BUFFER_SIZE)` with a size of 1024. That meant sample #1025 was being written to index 1024 on every frame.
- **Fix:** The check was tightened to `>=`, which closes the gap cleanly.

### 3. ✅ Stack buffer overflow in `ft8_decode()`
- **File:** `src/decode_ft8.cpp`
- **What was going on:** A 20‑byte stack buffer was being filled with a formatted string that could easily exceed that size. In plain English: a short buffer was being asked to hold too much, and that is a classic crash risk.
- **Fix:** The buffer was expanded to 70 bytes to match WSJT‑X’s expected sizing.

### 4. ✅ `sprintf` into 22‑byte buffers (two more sites)
- **Files:** `src/main.cpp`, `src/ADIF.cpp`
- **What was going on:** `received_message[22]` was too small for the strings it was asked to format. The actual payload could be much longer than that.
- **Fix:** Both buffers were increased to 40 bytes so the messages fit without stomping memory.

### 5. ✅ Small display buffer overflows
- **File:** `src/Display.cpp`
- **What was going on:** A couple of display buffers were a little too snug for their format strings. One message format could overflow a 13‑byte buffer, and one time string could overflow a 11‑byte buffer by a byte or two.
- **Fix:** Both were bumped to 16 bytes, which gives a bit of breathing room and keeps things safe.

### 6. ✅ INI parser values ran past the line end
- **File:** `src/ini.cpp`
- **What was going on:** During parsing, the value copy was being bounded by the end of the whole buffer instead of the end of the current line. That meant values could accidentally swallow the trailing newline and even the start of the next section. In other words, it was reading too far.
- **Fix:** The value copy now stops at the current line end, which is the correct boundary.

### 7. ✅ 128‑tap Q15 FIR accumulated in `int32_t`
- **File:** `src/SDR_Audio.cpp`
- **What was going on:** This was the kind of subtle numerical overflow bug that only shows up under real signal conditions. The accumulator could exceed `INT32_MAX` and corrupt the decode path.
- **Fix:** The accumulators were widened to `int64_t`, which is a much safer fit for the math involved.

### 8. ✅ `strncpy` without guaranteed NUL termination
- **File:** `src/autoseq_engine.cpp`
- **What was going on:** Several copied strings were using `strncpy`, which does not reliably append a terminating `\0` when the source is shorter than the destination. That could leave stale bytes behind and scramble TX text or ADIF output.
- **Fix:** Those copies were switched to `snprintf(dst, sizeof(dst), "%s", src)`, which is the safer pattern here. The two length‑checked copies in `write_worked_qso()` were already okay.

### 9. ✅ Out‑of‑bounds read in `process_selected_Station()`
- **File:** `src/decode_ft8.cpp`
- **What was going on:** The logic allowed an index of `new_decoded[decoded_count]` when it should have stayed inside the valid range.
- **Fix:** The condition was tightened so it now stops at the correct bound.

### 10. ✅ Audio capture stalled during decode
- **Files:** `src/main.cpp`, `src/SDR_Audio.cpp`, `include/SDR_Audio.h`
- **What was going on:** The code would stop draining the 32 kHz I2S stream while LDPC decode was running. That sounds harmless, but it can overflow the DMA and drift the 15‑second frame boundary, which is exactly the sort of thing that makes FT8 slots go sideways.
- **Fix:** Audio is now drained every loop iteration, and the FT8 capture is gated cleanly so samples are discarded during decode. A new `reset_audio_capture()` call also zeros the counters at each slot boundary so each frame starts cleanly.

### 11. ✅ Band switching was display‑only
- **Files:** `src/user_io.cpp`, `src/main.cpp`
- **What was going on:** Pressing `Bnd-` or `Bnd+` updated the visible band number, but it never actually retuned the SI5351 or persisted the new setting. Worse, `setup()` was hardcoding `start_freq = 14074`, which silently discarded the EEPROM band value.
- **Fix:** New `apply_band_change()` logic updates the frequency, refreshes the display, re‑centers the cursor, retunes the RX LO, and saves the band. The hardcoded startup frequency was removed.

### 12. ✅ EEPROM settings never committed
- **File:** `src/options.cpp`
- **What was going on:** Writes were happening, but they were never being committed to EEPROM. On a loss of power, the settings could simply vanish.
- **Fix:** `EEPROM.commit()` is now called after both write paths.

### 13. ✅ Slot‑state toggle made explicit
- **File:** `src/main.cpp`
- **What was going on:** `slot_state ^= 1;` was a blind toggle. That only works if every 15‑second boundary is seen exactly right, which is a little too optimistic in real life.
- **Fix:** `slot_state` is now set directly from `current_slot`, so it self‑corrects if a boundary is missed.

### 14. ✅ NTP sync could hang boot forever; Wi‑Fi pins set too late
- **File:** `src/Display.cpp` (`setup_RTC`)
- **What was going on:** The startup code could get stuck in unbounded SNTP waits, and the custom Tab5 Wi‑Fi pins were being configured too late. That can leave the radio with a half‑configured network state.
- **Fix:** Both sync loops now time out after 30 seconds, and `WiFi.setPins()` is called before `configTzTime()`.

### 15. ✅ `M5.begin()` called three times
- **Files:** `src/Display.cpp`, `src/ADIF.cpp`, `src/main.cpp`
- **What was going on:** The display setup and log setup were both calling `M5.begin()` again, on top of the main setup call. That meant the board was being initialized multiple times with slightly different config values.
- **Fix:** Those duplicate calls were removed, and `external_rtc = true` was moved into the single `M5.begin(cfg)` in `setup()`.

### 16. ✅ `MAX_MSG_LEN`/`MAX_LINE_LEN` too small (latent TX truncation)
- **File:** `include/autoseq_engine.h`
- **What was going on:** The message buffers were only 22 bytes wide, but a full FT8 TX line can be much longer than that. In practice, that meant the transmit text could get truncated silently.
- **Fix:** Both were increased to 40 bytes, and the ripple effects were updated across the codebase where those buffers were used.

### 17. ✅ `autoseq_on_touch()` compared with wrong length
- **File:** `src/autoseq_engine.cpp`
- **What was going on:** The “addressed to me” check was comparing against the locator length instead of the callsign length, which could misidentify the addressee.
- **Fix:** It now uses `CALLSIGN_SIZE` instead.
- **Note:** The TE1 override path was re‑checked and was already behaving correctly; no change was needed there.

### 18. ✅ `handleMsgToggle()` was a no‑op
- **File:** `src/user_io.cpp`
- **What was going on:** The “Msg” button only reset `Free_Index` and never actually updated the CQ mode state, so the POTA/SOTA/QRP CQ options were effectively unreachable from the UI.
- **Fix:** The toggle now sets `CQ_Mode_Index = state ? 2 : 0` and clears `free_text`, which matches the button behavior and labels.

### 19. ✅ Touch handlers ran 11× per touch
- **File:** `src/user_io.cpp` (`process_user_io`)
- **What was going on:** The touch handlers were sitting inside a per‑control loop, which meant they were being executed repeatedly for a single touch.
- **Fix:** They were moved out of the loop so they now fire once per touch, as expected.

### 20. ✅ Unbounded `sprintf` chain in `write_ADIF_Log()`
- **File:** `src/ADIF.cpp`
- **What was going on:** The ADIF log line was being assembled with a long chain of `sprintf` calls that didn’t respect the remaining buffer space.
- **Fix:** It was rewritten as a bounded `snprintf` chain with proper remaining-size checks.

### 21. ✅ ADIF/misc display fixes
- **File:** `src/ADIF.cpp`, `src/Display.cpp`
- **Things fixed:**
  - The log filename path had an extra leading space: `"/ %s.adi"` became `"/%s.adi"`.
  - The time format `"%2.2i"` was invalid and became `"%02d"`.
  - `WIFI_SSID` and `WIFI_PASSWORD` were expanded from 20 bytes to 33 bytes to cover the max credential lengths safely.

### 22. ✅ `trim_back()` UB on empty strings
- **File:** `src/text.cpp`
- **What was going on:** The function was doing `strlen(str) - 1` even when the string was empty, which can underflow and trigger undefined behavior.
- **Fix:** It now exits early for empty strings.

### 23. ✅ Yellow "own TX" highlight marked every line
- **File:** `src/decode_ft8.cpp` (`display_messages`)
- **What was going on:** The `was_txing` check was turning every decoded line yellow instead of only the one that actually matched the station call.
- **Fix:** The highlight now only appears when `call_from` matches `Station_Call`, which is the WSJT‑X behavior I expect.

### 24. ✅ Dead code removed
- **Files:** `src/button.cpp`/`include/button.h`, `src/gen_ft8.cpp`/`include/gen_ft8.h`
- **Removed after verifying there were zero references:** `testButton`, `reset_buttons`, `update_CQFree_button`, `erase_Cal_Display`, `EEPROMWriteInt`, `EEPROMReadInt`, `clear_reply_message_box`, and `erase_CQ`.

### 25. ✅ Build configuration
- **File:** `platformio.ini`, `include/main.h`
- **Fixed:** `build_type` was switched from debug to release, `CORE_DEBUG_LEVEL` was reduced from 5 to 0, and the redundant `#pragma once` was removed from `main.h` while keeping the guard pattern intact.

## Noted, not changed

| `set_Attenuator_Gain()` / `set_RF_Gain()` are empty stubs | `src/button.cpp` | `tune_On_sequence()` calls the attenuator; only the relay mutes TX. This is probably fine if the hardware doesn’t actually have an attenuator — if it does, remove the calls. |
| `Band_Minimum` is a zero‑initialized global, never explicitly set | `src/button.cpp` | `Bnd-` correctly does nothing at 40 m (lowest band), so it works as‑is. |
| Two `sinf()` per 32 kHz sample in the mixer | `src/SDR_Audio.cpp` | `lo_sample_index` only takes 16 values, so a 16‑entry LUT would be faster. It isn’t a big deal on the P4, so it’s not urgent. |
| Raw SGTL5000 register sequence with `delay(400)` | `src/SDR_Audio.cpp` | It’s a bit brittle and magic‑numbery. It would be nice to document the intended gain/mic setup more clearly. |
| `static void compute_radix2_fft(...)` defined in a header | `include/Process_DSP.h` | Every includer compiles its own private copy. Moving it to `Process_DSP.cpp` would be cleaner, but it’s not a functional blocker. |
| `display_messages()` parameter shadows the global `new_decoded` | `src/decode_ft8.cpp` | Harmless, but a bit confusing. Renaming it would make the code easier to read. |

The short version? This is a strong port with a few real bug fixes under the hood, and it now looks much more stable and trustworthy than it did at the start.
