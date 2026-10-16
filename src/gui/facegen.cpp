#include "facegen.h"

#include <cmath>

namespace facegen {
namespace {

// Palette indices. Kept as an enum so compose() reads clearly.
enum : uint8_t {
  kTransparent = 0,
  kOutline,     // darkest line work (pupils, frames, outlines)
  kSkinLight,
  kSkinMid,
  kSkinDark,
  kSkinDeep,
  kHairBlack,
  kHairBrown,
  kHairBlond,
  kHairRed,
  kHairGrey,
  kEyeWhite,
  kEyeIris,
  kMouth,
  kMouthLow,
  kShirtLight,
  kShirtMid,
  kShirtDark,
  kHat,
  kGlass,
  kGold,
  kMark,
  kPatch,
  kFreckle,
  kCount
};

const Color kPalette[kCount] = {
    /* kTransparent */ {0, 0, 0, 0},
    /* kOutline     */ {24, 20, 24, 255},
    /* kSkinLight   */ {245, 206, 170, 255},
    /* kSkinMid     */ {222, 178, 138, 255},
    /* kSkinDark    */ {176, 124, 84, 255},
    /* kSkinDeep    */ {120, 78, 48, 255},
    /* kHairBlack   */ {32, 26, 22, 255},
    /* kHairBrown   */ {96, 60, 32, 255},
    /* kHairBlond   */ {216, 182, 96, 255},
    /* kHairRed     */ {168, 72, 40, 255},
    /* kHairGrey    */ {186, 186, 186, 255},
    /* kEyeWhite    */ {246, 246, 250, 255},
    /* kEyeIris     */ {54, 84, 140, 255},
    /* kMouth       */ {168, 72, 72, 255},
    /* kMouthLow    */ {144, 60, 60, 255},
    /* kShirtLight  */ {206, 210, 220, 255},
    /* kShirtMid    */ {92, 112, 152, 255},
    /* kShirtDark   */ {58, 58, 78, 255},
    /* kHat         */ {44, 44, 52, 255},
    /* kGlass       */ {168, 200, 220, 160},
    /* kGold        */ {222, 190, 82, 255},
    /* kMark        */ {36, 24, 16, 255},
    /* kPatch       */ {220, 220, 230, 255},
    /* kFreckle     */ {196, 140, 92, 255},
};

// A tiny indexed raster with the handful of primitives the face needs.
struct Raster {
  int w, h;
  std::vector<uint8_t> px;

  Raster(int width, int height)
      : w(width), h(height), px(static_cast<std::size_t>(width) * height, kTransparent) {}

  void set(int x, int y, uint8_t c) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    px[static_cast<std::size_t>(y) * w + x] = c;
  }

  void rect(int x0, int y0, int x1, int y1, uint8_t c) {
    for (int y = y0; y <= y1; ++y)
      for (int x = x0; x <= x1; ++x) set(x, y, c);
  }

  void hline(int x0, int x1, int y, uint8_t c) { rect(x0, y, x1, y, c); }
  void vline(int x, int y0, int y1, uint8_t c) { rect(x, y0, x, y1, c); }

  void box(int x0, int y0, int x1, int y1, uint8_t c) {
    hline(x0, x1, y0, c);
    hline(x0, x1, y1, c);
    vline(x0, y0, y1, c);
    vline(x1, y0, y1, c);
  }

  void triangleTop(double cx, double cy, double w, uint8_t c) {
    const int y0 = static_cast<int>(std::floor(cy - w * 0.5));
    const int y1 = static_cast<int>(std::ceil(cy + w * 0.25));
    for (int y = y0; y <= y1; ++y) {
      double t = (y - (cy - w * 0.5)) / (w * 0.75);
      if (t < 0.0) continue;
      if (t > 1.0) t = 1.0;
      const double half = w * (1.0 - t) * 0.5;
      const int x0r = static_cast<int>(std::floor(cx - half));
      const int x1r = static_cast<int>(std::ceil(cx + half));
      for (int x = x0r; x <= x1r; ++x) set(x, y, c);
    }
  }

  void ellipse(double cx, double cy, double rx, double ry, uint8_t c) {
    const int y0 = static_cast<int>(std::floor(cy - ry));
    const int y1 = static_cast<int>(std::ceil(cy + ry));
    const int x0 = static_cast<int>(std::floor(cx - rx));
    const int x1 = static_cast<int>(std::ceil(cx + rx));
    for (int y = y0; y <= y1; ++y)
      for (int x = x0; x <= x1; ++x) {
        const double dx = (x + 0.5 - cx) / rx;
        const double dy = (y + 0.5 - cy) / ry;
        if (dx * dx + dy * dy <= 1.0) set(x, y, c);
      }
  }
};

// Deterministic 32-bit hash (splitmix-style finaliser).
uint32_t mix(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

// Returns the base and shade skin indices for a skin id.
void skinTones(uint8_t id, uint8_t& base, uint8_t& shade) {
  switch (id % 4) {
    case 0:  base = kSkinLight; shade = kSkinMid;  break;
    case 1:  base = kSkinMid;   shade = kSkinDark; break;
    case 2:  base = kSkinDark;  shade = kSkinDeep; break;
    default: base = kSkinDeep;  shade = kSkinDeep; break;
  }
}

uint8_t hairColor(uint8_t id) {
  static const uint8_t kHair[5] = {kHairBlack, kHairBrown, kHairBlond, kHairRed, kHairGrey};
  return kHair[id % 5];
}

// The Face struct only carries one hair id, so colour and style are carved out
// of it independently: the low digit picks the colour, the next picks the cut.
uint8_t hairStyle(uint8_t id) {
  return static_cast<uint8_t>((id / 5) % 12);
}

uint8_t eyeColor(uint8_t id) {
  // Small variations around the iris colour.
  static const uint8_t kIris[4] = {kEyeIris, kHairBrown, kHairBlond, kHairGrey};
  return kIris[id % 4];
}

uint8_t shirtColor(uint8_t id) {
  static const uint8_t kShirt[3] = {kShirtLight, kShirtMid, kShirtDark};
  return kShirt[id % 3];
}

}  // namespace

const std::vector<Color>& palette() {
  static const std::vector<Color> pal(kPalette, kPalette + kCount);
  return pal;
}

Image compose(const Face& face) {
  Raster r(kWidth, kHeight);

  uint8_t skin = kSkinLight;
  uint8_t skinShade = kSkinMid;
  skinTones(face.skin_id, skin, skinShade);

  const uint8_t hair = hairColor(face.hair_id);
  const uint8_t shirt = shirtColor(face.clothing_id);
  const bool female = (face.gender_id % 2) != 0;

  // Hair style: 0 short, 1 long, 2 bald, 3 afro, 4 mohawk, 5 crew cut, 6 side part,
  // 7 mullet, 8 pompadour, 9 slicked back, 10 top knot, 11 widow's peak.
  uint8_t style = hairStyle(face.hair_id);
  if (female && style == 2) style = 1;

  // --- shoulders / body -----------------------------------------------------
  r.ellipse(23.5, 56.0, 20.0, 18.0, shirt);
  // collar shadow
  r.ellipse(23.5, 44.0, 8.0, 4.0, skinShade);

  // --- neck -----------------------------------------------------------------
  r.rect(20, 30, 27, 40, skin);
  r.rect(20, 30, 27, 32, skinShade);

  // --- hair (mass + sides), then the face is painted over its inner edge ----
  switch (style) {
    case 1:  // long
      r.ellipse(23.5, 18.0, 14.0, 16.0, hair);
      r.rect(9, 18, 15, 41, hair);
      r.rect(32, 18, 38, 41, hair);
      break;
    case 2:  // bald: just sideburns
      r.rect(11, 25, 14, 31, hair);
      r.rect(33, 25, 36, 31, hair);
      break;
    case 3:  // afro
      r.ellipse(23.5, 12.0, 18.0, 16.0, hair);
      r.ellipse(6.5, 22.0, 5.0, 7.0, hair);
      r.ellipse(40.5, 22.0, 5.0, 7.0, hair);
      break;
    case 4:  // mohawk
      r.rect(20, 1, 27, 16, hair);
      break;
    case 5:  // crew cut
      r.ellipse(23.5, 17.0, 13.5, 11.0, hair);
      break;
    case 6:  // side part
      r.ellipse(23.5, 18.0, 13.5, 12.0, hair);
      r.rect(23, 12, 38, 24, hair);
      break;
    case 7:  // mullet
      r.ellipse(23.5, 18.0, 13.0, 12.0, hair);
      r.rect(9, 22, 15, 38, hair);
      r.rect(32, 22, 38, 38, hair);
      break;
    case 8:  // pompadour
      r.ellipse(23.5, 16.0, 14.0, 12.0, hair);
      r.rect(17, 5, 36, 14, hair);
      break;
    case 9:  // slicked back
      r.ellipse(23.5, 17.0, 13.5, 12.0, hair);
      r.rect(11, 10, 14, 26, hair);
      r.rect(33, 10, 36, 26, hair);
      break;
    case 10: // top knot
      r.ellipse(23.5, 18.0, 12.5, 11.0, hair);
      r.rect(17, 2, 29, 10, hair);
      r.ellipse(23.0, 6.0, 6.0, 4.0, hair);
      break;
    case 11: // widow's peak
      r.ellipse(23.5, 18.0, 13.5, 12.0, hair);
      r.triangleTop(23.5, 7.0, 13.0, hair);
      break;
    default:  // 0 == short
      r.ellipse(23.5, 18.0, 13.5, 11.0, hair);
      break;
  }

  // --- head -----------------------------------------------------------------
  r.ellipse(23.5, 21.0, 12.5, 15.0, skin);
  // Ears.
  r.ellipse(9.5, 22.0, 2.6, 3.6, skin);
  r.ellipse(37.5, 22.0, 2.6, 3.6, skin);

  // Forehead hairline. Bald shows bare skin; the mohawk/topknot keep narrow strip.
  if (style == 4) {
    r.rect(20, 1, 27, 14, hair);
  } else if (style == 10) {
    r.rect(17, 2, 29, 12, hair);
    r.ellipse(23.0, 6.0, 6.0, 4.0, hair);
  } else if (style != 2 && style != 5 && style != 9) {
    if (style == 11) {
      r.triangleTop(23.5, 9.0, 14.0, hair);
    } else {
      r.ellipse(23.5, 9.5, 13.0, 6.5, hair);
    }
  } else if (style == 5) {
    r.ellipse(23.5, 10.0, 12.5, 5.5, hair);
  } else if (style == 9) {
    r.ellipse(23.5, 10.0, 12.5, 6.0, hair);
  }

  // --- eyebrows -------------------------------------------------------------
  {
    const uint8_t mood = static_cast<uint8_t>((face.eye_id * 3 + face.accessory_id * 5 + face.hair_id * 2) % 7);
    if (mood == 2 || mood == 5) {
      // raised
      r.rect(14, 18, 21, 19, hair);
      r.rect(26, 18, 33, 19, hair);
    } else if (mood == 3 || mood == 4) {
      // furrowed
      r.rect(14, 20, 21, 20, hair);
      r.rect(26, 20, 33, 20, hair);
      r.set(14, 19, hair);
      r.set(21, 19, hair);
      r.set(26, 19, hair);
      r.set(33, 19, hair);
    } else {
      r.rect(14, 19, 21, 20, hair);
      r.rect(26, 19, 33, 20, hair);
    }
  }

  // --- eyes -----------------------------------------------------------------
  const uint8_t iris = eyeColor(face.eye_id);
  for (int ex : {18, 29}) {
    r.ellipse(ex + 0.5, 23.5, 3.2, 2.2, kEyeWhite);
    r.ellipse(ex + 0.5, 23.5, 1.5, 1.6, iris);
    r.set(ex, 23, kOutline);
    r.set(ex + 1, 23, kOutline);
  }

  // --- nose -----------------------------------------------------------------
  r.vline(23, 24, 28, skinShade);
  r.rect(21, 28, 25, 28, skinShade);

  // --- mouth ----------------------------------------------------------------
  {
    // Mood: combine components to get a small emotion index.
    const uint8_t mood = static_cast<uint8_t>((face.eye_id * 3 + face.accessory_id * 5 + face.hair_id * 2) % 7);
    if (mood == 1) {
      // slight smile
      r.rect(19, 32, 28, 32, kMouth);
      r.set(18, 31, kMouth);
      r.set(29, 31, kMouth);
    } else if (mood == 2) {
      // big smile
      r.rect(19, 31, 28, 32, kMouth);
      r.set(18, 30, kMouth);
      r.set(29, 30, kMouth);
    } else if (mood == 3) {
      // frown
      r.rect(19, 32, 28, 32, kMouthLow);
      r.set(18, 33, kMouthLow);
      r.set(29, 33, kMouthLow);
    } else if (mood == 4) {
      // grimace
      r.rect(20, 31, 27, 32, kMouthLow);
      r.set(19, 30, kMouth);
      r.set(28, 30, kMouth);
    } else if (mood == 5) {
      // surprised
      r.rect(21, 31, 26, 33, kMouthLow);
      r.set(20, 30, kMouth);
      r.set(27, 30, kMouth);
    } else if (mood == 6) {
      // tight lips
      r.rect(20, 32, 27, 32, kMouth);
    } else {
      // neutral
      r.rect(19, 32, 28, 32, kMouth);
      r.set(18, 31, kMouth);
      r.set(29, 31, kMouth);
    }
  }

  // Expand accessory space to include facial hair, scars, marks and eyepatch.
  // Keep scars/patches rare so they're "defining marks".
  uint8_t accBase = static_cast<uint8_t>(face.accessory_id % 16);
  uint8_t accessory = accBase;
  if (female && (accessory == 3 || accessory == 5 || accessory == 6 || accessory == 7 || accessory == 8 || accessory == 9)) {
    accessory = (accBase == 9) ? 10 : (accBase == 6 ? 4 : (accBase == 8 ? 11 : 0));
  }
  // Rare marks are uncommon for randomly generated faces, but we still want
  // the lineup to show variety when callers ask for specific seeds.
  const bool rareMark = (accBase == 10 || accBase == 11 || accBase == 13 || accBase == 14 || accBase == 15);
  if (rareMark && ((face.accessory_id & 15) != 0)) {
    accessory = 0;
  }

  // --- accessory ------------------------------------------------------------
  switch (accessory) {
    case 1:  // glasses
      r.rect(15, 21, 21, 26, kGlass);
      r.box(15, 21, 21, 26, kOutline);
      r.rect(25, 21, 31, 26, kGlass);
      r.box(25, 21, 31, 26, kOutline);
      r.hline(22, 24, 23, kOutline);
      break;
    case 2:  // hat
      r.rect(11, 1, 36, 9, kHat);
      r.rect(6, 9, 41, 12, kHat);
      r.hline(11, 36, 1, kOutline);
      break;
    case 3:  // full beard
      r.ellipse(23.5, 33.0, 11.5, 8.0, hair);
      r.rect(19, 30, 28, 30, hair);  // moustache
      r.rect(19, 32, 28, 32, kMouthLow);
      r.set(18, 32, kMouthLow);
      r.set(29, 32, kMouthLow);
      r.set(19, 31, kMouth);
      r.set(28, 31, kMouth);
      break;
    case 4:  // earrings
      r.rect(9, 24, 10, 25, kGold);
      r.rect(37, 24, 38, 25, kGold);
      break;
    case 5:  // moustache only
      r.rect(19, 30, 28, 30, hair);
      break;
    case 6:  // goatee
      r.ellipse(23.5, 35.0, 6.0, 4.0, hair);
      r.rect(19, 30, 28, 30, hair);
      r.rect(19, 32, 28, 32, kMouthLow);
      r.set(18, 32, kMouthLow);
      r.set(29, 32, kMouthLow);
      break;
    case 7:  // van dyke
      r.rect(21, 32, 26, 36, hair);
      r.ellipse(23.5, 36.0, 2.8, 2.2, hair);
      r.rect(19, 30, 28, 30, hair);
      r.rect(19, 32, 28, 32, kMouthLow);
      r.set(18, 32, kMouthLow);
      r.set(29, 32, kMouthLow);
      break;
    case 8:  // chin strap + sideburns
      r.rect(10, 26, 12, 36, hair);
      r.rect(35, 26, 37, 36, hair);
      r.rect(10, 36, 37, 38, hair);
      r.rect(19, 30, 28, 30, hair);
      r.rect(19, 32, 28, 32, kMouthLow);
      r.set(18, 32, kMouthLow);
      r.set(29, 32, kMouthLow);
      break;
    case 9:  // soul patch
      r.rect(22, 33, 25, 35, hair);
      break;
    case 10: // scar across cheek/forehead
      r.vline(12, 18, 22, kMark);
      r.hline(11, 13, 17, kMark);
      break;
    case 11: // eyepatch
      r.rect(14, 20, 22, 26, kPatch);
      r.box(14, 20, 22, 26, kOutline);
      r.hline(22, 24, 23, kOutline);
      r.rect(26, 22, 28, 24, kPatch);
      break;
    case 12: // mole
      r.set(32, 28, kMark);
      r.set(33, 28, kMark);
      break;
    case 13: // facial scar vertical
      r.vline(34, 20, 28, kMark);
      break;
    case 14: // old scar diagonal
      r.set(13, 24, kMark);
      r.set(14, 25, kMark);
      r.set(15, 26, kMark);
      break;
    case 15: // mole on chin
      r.set(21, 34, kMark);
      break;
    default:  // none
      break;
  }

  // --- freckles / age-ish subtle marks -------------------------------------
  {
    const uint8_t m = static_cast<uint8_t>((face.skin_id * 7 + face.hair_id * 3 + face.eye_id * 5) % 16);
    if (m < 4 && skin == kSkinLight) {
      r.set(16, 25, kFreckle);
      r.set(19, 26, kFreckle);
      r.set(28, 27, kFreckle);
      r.set(31, 26, kFreckle);
    } else if (m < 6 && skin == kSkinMid) {
      r.set(17, 26, kFreckle);
      r.set(30, 27, kFreckle);
    }
  }

  Image out;
  out.width = r.w;
  out.height = r.h;
  out.indices = std::move(r.px);
  return out;
}

Face makeFace(uint32_t seed) {
  uint32_t h = seed * 2654435761u + 1013904223u;
  auto next = [&h]() {
    h = mix(h);
    return h;
  };
  Face f{};
  f.gender_id = static_cast<uint8_t>(next() % 2);
  f.hair_id = static_cast<uint8_t>(next() % 60);   // 12 styles x 5 colours
  f.eye_id = static_cast<uint8_t>(next() % 6);
  f.skin_id = static_cast<uint8_t>(next() % 4);
  f.clothing_id = static_cast<uint8_t>(next() % 3);
  f.accessory_id = static_cast<uint8_t>(next() % 16);
  return f;
}

Face chiefFace() {
  Face f{};
  f.gender_id = 0;
  f.hair_id = 4;       // grey
  f.eye_id = 0;        // blue
  f.skin_id = 1;
  f.clothing_id = 2;   // dark
  f.accessory_id = 0;
  return f;
}

Face playerFace() {
  Face f{};
  f.gender_id = 0;
  f.hair_id = 1;       // brown
  f.eye_id = 0;
  f.skin_id = 0;
  f.clothing_id = 0;
  f.accessory_id = 0;
  return f;
}

Face candidateFace(int index) {
  // A fixed offset keeps the lineup stable across runs; index 0 is skipped so
  // the "player" face never appears as a suspect by accident.
  // Maximise variation between the 15 candidates.
  uint32_t seed = 0xC0FFEE55u ^ static_cast<uint32_t>(index + 1) * 0xA5A5A5A5u;
  seed ^= static_cast<uint32_t>(index + 1) * 131542391u;
  seed ^= (static_cast<uint32_t>(index + 1) << 13) ^ (static_cast<uint32_t>(index + 1) << 19);
  seed ^= (static_cast<uint32_t>(index + 1) << 7) ^ (static_cast<uint32_t>(index + 1) << 23);
  // Permute component bits more aggressively.
  Face f = makeFace(mix(seed));
  f.hair_id = static_cast<uint8_t>((f.hair_id + static_cast<uint8_t>(index * 31)) % 60);
  f.eye_id = static_cast<uint8_t>((f.eye_id + static_cast<uint8_t>(index * 17)) % 6);
  f.skin_id = static_cast<uint8_t>((f.skin_id + static_cast<uint8_t>(index * 23)) % 4);
  f.clothing_id = static_cast<uint8_t>((f.clothing_id + static_cast<uint8_t>(index * 13)) % 3);
  f.accessory_id = static_cast<uint8_t>((f.accessory_id + static_cast<uint8_t>(index * 7)) % 16);
  f.gender_id = static_cast<uint8_t>(index % 2);
  return f;
}

Face makeFaceFromParticipant(const CaseParticipant& participant) {
  // Identity: a strict FNV-1a over the role name. Byte-wise and fixed width, so
  // the same participant hashes identically on every platform and build.
  uint32_t idHash = 2166136261u;
  for (const unsigned char c : participant.role) {
    idHash ^= static_cast<uint32_t>(c);
    idHash *= 16777619u;
  }

  const uint32_t rank = participant.rank;
  const uint32_t exposure = participant.exposure;
  const uint32_t clue = participant.clue_type;
  const uint32_t master = participant.mastermind ? 1u : 0u;

  // Weighted blend of the selected fields. The identity term dominates; the
  // others add smaller contributions, and every term folds the identity hash in
  // so a participant stays recognisable rather than being decided by the field
  // values alone.
  const uint32_t h = idHash * 8u +
                     mix(idHash ^ (rank * 0x9E3779B9u)) * 4u +
                     mix(idHash ^ (exposure * 0x85EBCA6Bu)) * 4u +
                     mix(idHash ^ (clue * 0xC2B2AE35u)) * 2u +
                     mix(idHash ^ (master * 0x517CC1B7u)) * 4u;

  // The base look is the identity alone.
  Face f = makeFace(idHash);

  // Seniority: rising through the ranks and staying active take their toll, so
  // high rank/exposure push the hair toward grey.
  const unsigned age = rank * 8u + exposure;
  if (age >= 40u || (age >= 24u && ((h >> 5) & 1u) != 0u)) {
    f.hair_id = static_cast<uint8_t>((f.hair_id / 5u) * 5u + 4u);  // colour = grey
  }

  // Clue type leaves a light imprint on the wardrobe and the eyes, giving each
  // organisation's people a faint shared look.
  f.clothing_id = static_cast<uint8_t>((f.clothing_id + clue) % 3u);
  f.eye_id = static_cast<uint8_t>((f.eye_id + clue) % 6u);

  // A heavily seen face more often carries a defining mark, and the mastermind
  // always gets a distinctive one so they stand out in the lineup.
  if (participant.mastermind) {
    static const uint8_t kTells[] = {11, 13, 14, 2, 1};  // eyepatch, scars, hat, glasses
    f.accessory_id = kTells[(h >> 7) % (sizeof(kTells) / sizeof(kTells[0]))];
  } else if (exposure >= 8u && ((h >> 11) & 1u) != 0u) {
    static const uint8_t kMarks[] = {10, 12, 13, 14, 15};  // scars and moles
    f.accessory_id = kMarks[(h >> 13) % (sizeof(kMarks) / sizeof(kMarks[0]))];
  }

  return f;
}

}  // namespace facegen
