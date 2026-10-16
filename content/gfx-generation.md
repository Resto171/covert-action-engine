There seem to be a variety of moments where a face is needed:

* clues
* watch building
* debrief
* arrest

Some of the variation that occurs involves black/white vs color and clothing.

## Current implementation

The engine does not ship the original `.PIC` artwork. Portraits are generated
procedurally from the six `Face` components (gender, hair, eyes, skin, clothing,
accessory); see `src/gui/facegen.h` / `src/gui/facegen.cpp`. Those are the same
components the original face generator varied.
