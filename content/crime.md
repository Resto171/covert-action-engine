## Crime file format

_Layout verified against all thirteen shipped `CRIME*.DTA` files. Each file is
exactly `4 + pp*48 + ee*44 + 4*18` bytes, stored in the order header,
participants, events, object slots._

### Header

    pp 00 ee 00

- pp = # Participants
- ee = # Events + 1 (the final event is a blank terminator)

### Participant entry

    FF FF xx 00 nn nn nn nn nn nn nn nn nn nn nn nn
    nn nn nn nn nn nn nn nn nn nn nn nn nn nn nn nn
    nn nn nn nn ?? ?? ?? mm 00 00 uu 00 rr 00 ?? 00

- xx = exposure field (always 0 for mastermind)
- nn = name of role (32 bytes)
- mm = mastermind if 1 (trailer byte 3)
- uu = clue type (trailer byte 6)
- rr = participant rank (trailer byte 8)

#### Clue Type
- 00 vehicle
- 01 weapon
- 02 address
- 03 airline ticket
- 04 telegram
- 05 money (hundreds USD)
- 06 money (thousands USD)
- 07 identity document

### Event entry (qty. ee)

    ll 00 00 00 cc 00 ss ss ss ss ss ss ss ss ss ss
    ss ss ss ss ss ss ss ss ss ss ss ss ss ss ss ss
    ss ss ss ss ss ss dd tt bb zz vv vv

_These offsets need to be double-checked!_

- ll = Link to person conducting entry
- cc = Case event/message ID
- ss = Event or message statement/description
- dd = Event destination/target/recipient
- tt = Type
- bb = Bitmask for received item (bit # = item #)
- zz = Bitmask for destroyed item
- vv = Value of event. Multiply by 40 to get score.

#### Types

- 02 Sent message (paired with 3)
- 03 Received message (paired with 2)
- 08 Met with (paired with 9)
- 09 Was met by (paired with 8)
- 20 Crime

### Blank event entry (qty. 1)

    FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00

### Object entry

There are always exactly four object slots (72 bytes) after the events; empty
slots have an empty name and an icon of `FF`.

    oo oo oo oo oo oo oo oo oo oo oo oo oo oo oo oo
    ii FF

- ii = Icon ID

#### Icons

- 00 - Processed drugs
- 01 - Sniper rifle / Weapons
- 02 - Raw drugs
- 03 - Chemicals / Supplies
- 04 - Explosives
- 05 - Alarm Bypass / Product
- 06 - Photographs
- 07 - Hostage / Pilot / Escapee
- 08 - Bomb
- 09 - Payoff / $1,000,000 / Gold coin
- 0A - Photographs
- 0B - Camera
- 0C - Blueprints / Mission plans
- 0D - Stolen goods / Jeep / Sample item / Forgery / Equipment
- 0E - Passwords

## TODO

- How is timing determined?
- Minor vs major crimes?
- What are the unknown bytes? (`??`)

