#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Reader for the original game's crime template files (CRIME*.DTA).
//
// The layout below was verified against all thirteen shipped files, each of
// which is exactly `4 + pp*48 + ee*44 + 4*18` bytes:
//
//   offset 0    u16  participant count (pp)
//   offset 2    u16  event count (ee); the final event is a blank terminator
//   offset 4    pp * 48-byte participant records
//   ...         ee * 44-byte event records
//   ...         4  * 18-byte object slots (empty slots have icon 0xFF)
//
// The field names follow the project's content/crime.md notes. A few of the
// participant trailer bytes (marked "unknown" below) are kept verbatim until
// their meaning is pinned down.

namespace casefile {

inline constexpr std::size_t kHeaderSize = 4;
inline constexpr std::size_t kParticipantSize = 48;
inline constexpr std::size_t kEventSize = 44;
inline constexpr std::size_t kObjectSize = 18;
inline constexpr std::size_t kObjectSlots = 4;

inline constexpr std::size_t kNameLength = 32;   // participant/event text field
inline constexpr std::size_t kObjectNameLength = 16;

// Marks the blank event terminator and the empty object slots.
inline constexpr uint8_t kNone = 0xFF;

}  // namespace casefile


struct CaseParticipant {
    uint8_t exposure { 0 };   // 0 for the mastermind
    std::string role;         // role name (32-byte NUL-padded field)
    bool mastermind { false };
    uint8_t clue_type { 0 };
    uint8_t rank { 0 };
    // The full 12-byte trailer, kept as-is so nothing is silently dropped.
    std::array<uint8_t, 12> trailer {};

    // Trailer byte offsets that have been identified.
    static constexpr std::size_t kMastermindOffset = 3;
    static constexpr std::size_t kClueTypeOffset = 6;
    static constexpr std::size_t kRankOffset = 8;
};


struct CaseEvent {
    int participant { 0 };       // link to the participant conducting the event
    int message_id { 0 };
    std::string text;            // event/message description (32-byte field)
    int target { 0 };            // destination/recipient participant
    uint8_t type { 0 };
    uint8_t received_mask { 0 };  // received item bitmask (bit # = item #)
    uint8_t destroyed_mask { 0 }; // destroyed item bitmask
    int value { 0 };             // event value (score is value * 40)
};


struct CaseObject {
    std::string name;                       // object name (16-byte field)
    uint8_t icon { casefile::kNone };
};


struct CaseFile {
    std::vector<CaseParticipant> participants;
    std::vector<CaseEvent> events;    // excludes the blank terminator
    std::vector<CaseObject> objects;  // excludes empty slots
};


// Parse a crime file from memory. Returns false for a null/short buffer or when
// the declared counts do not fit, so truncated or foreign data is rejected
// instead of misread.
bool parseCaseFile(const uint8_t* data, std::size_t size, CaseFile& out);

// Read and parse a crime file from disk.
bool loadCaseFile(const std::string& path, CaseFile& out);
