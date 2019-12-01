x0xb0x - MarOS15 - MOD
======================
## Preamble

This branch contains some necessary changes to the x0xb0x MarOS15 firmware, which has already been adapted by Nordcode. The adjustments are largely compatible with the MarOS15 1.81 version (Nordcode) - but only for the x0xLarge CPU (Atmega2561) and include the following changes. The main source version on which the changes are based is in the 'Original' branch.

## Modifications

* fixed compiler warnings
* removed CRC stuff
* added c0nb0x (1.02) communication
* added more visuals in midiplay mode
* added antto's modified MVA note tracking algorithm in midiplay mode
* added holded notes release by transposed keys in midiplay mode
* fixed pattern "program change" in "advanced mode" (next, bank, visuals etc.)
* fixed note range for midiplay without octave loop
* fixed note range for pattern_play
* fixed run/stop handling issues

## Added USERC Settings #F Options:
-----------------------------------
* `Key #4 LED ON = ignores pattern selection by notes (fullrange trans)`
* `Key #7 LED ON = transposing down -1OCT for midiplay incoming notes`
* `Key #8 LED ON = instant program change`

## Notes

MarOS15 for x0xlarge CPU (Atmega2561).

Original code by Limor Fried - https://www.adafruit.com/ - http://www.ladyada.net/
Modifications by Sokkan + Mario1089 + Nordcore + Rio
