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
  kShirtLight,
  kShirtMid,
  kShirtDark,
  kHat,
  kGlass,
  kGold,
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
    /* kShirtLight  */ {206, 210, 220, 255},
    /* kShirtMid    */ {92, 112, 152, 255},
    /* kShirtDark   */ {58, 58, 78, 255},
    /* kHat         */ {44, 44, 52, 255},
    /* kGlass       */ {168, 200, 220, 160},
    /* kGold        */ {222, 190, 82, 255},
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
  return static_cast<uint8_t>((id / 5) % 5);
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

  // Hair style: 0 short, 1 long, 2 bald, 3 afro, 4 mohawk. Women get the long
  // style more often; a bald man stays bald.
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
    default:  // 0 == short
      r.ellipse(23.5, 18.0, 14.0, 16.0, hair);
      break;
  }

  // --- head -----------------------------------------------------------------
  r.ellipse(23.5, 21.0, 12.5, 15.0, skin);
  // Ears.
  r.ellipse(9.5, 22.0, 2.6, 3.6, skin);
  r.ellipse(37.5, 22.0, 2.6, 3.6, skin);

  // Forehead hairline. Bald shows bare skin; the mohawk keeps a narrow strip.
  if (style == 4) {
    r.rect(20, 1, 27, 14, hair);
  } else if (style != 2) {
    r.ellipse(23.5, 9.5, 13.0, 6.5, hair);
  }

  // --- eyebrows -------------------------------------------------------------
  r.rect(14, 19, 21, 20, hair);
  r.rect(26, 19, 33, 20, hair);

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
  r.rect(19, 32, 28, 32, kMouth);
  r.set(18, 31, kMouth);
  r.set(29, 31, kMouth);

  // Facial hair is not drawn on female faces; swap it for earrings instead.
  uint8_t accessory = static_cast<uint8_t>(face.accessory_id % 6);
  if (female && (accessory == 3 || accessory == 5)) accessory = 4;

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
    case 3:  // beard
      r.ellipse(23.5, 33.0, 11.5, 7.5, hair);
      r.rect(19, 30, 28, 30, hair);  // moustache
      // re-cut the mouth so it stays visible
      r.rect(19, 32, 28, 32, kMouth);
      r.set(18, 32, kMouth);
      r.set(29, 32, kMouth);
      break;
    case 4:  // earrings
      r.rect(9, 24, 10, 25, kGold);
      r.rect(37, 24, 38, 25, kGold);
      break;
    case 5:  // moustache only
      r.rect(19, 30, 28, 30, hair);
      break;
    default:  // none
      break;
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
  f.hair_id = static_cast<uint8_t>(next() % 25);
  f.eye_id = static_cast<uint8_t>(next() % 4);
  f.skin_id = static_cast<uint8_t>(next() % 4);
  f.clothing_id = static_cast<uint8_t>(next() % 3);
  f.accessory_id = static_cast<uint8_t>(next() % 6);
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
  return makeFace(0x00C0FFEEu + static_cast<uint32_t>(index + 1) * 0x9E3779B9u);
}

}  // namespace facegen
