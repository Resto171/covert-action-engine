#include "casefile.h"

#include <fstream>
#include <utility>

namespace {

uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

// Read a fixed-width, NUL-padded string, stopping at the first NUL byte or the
// end of the field.
std::string readFixedString(const uint8_t* p, std::size_t max_len) {
    std::size_t len = 0;
    while (len < max_len && p[len] != 0) {
        ++len;
    }
    return std::string(reinterpret_cast<const char*>(p), len);
}

}  // namespace


bool parseCaseFile(const uint8_t* data, std::size_t size, CaseFile& out) {
    if (data == nullptr || size < casefile::kHeaderSize) {
        return false;
    }

    const std::size_t participants = readU16LE(data);
    const std::size_t events = readU16LE(data + 2);
    const std::size_t fixed = casefile::kHeaderSize
                            + participants * casefile::kParticipantSize
                            + events * casefile::kEventSize;
    if (size < fixed) {
        return false;  // declared counts do not fit the buffer
    }

    CaseFile file;
    std::size_t offset = casefile::kHeaderSize;

    file.participants.reserve(participants);
    for (std::size_t i = 0; i < participants; ++i) {
        const uint8_t* rec = data + offset;
        CaseParticipant p;
        p.exposure = rec[2];
        p.role = readFixedString(rec + 4, casefile::kNameLength);
        for (std::size_t k = 0; k < p.trailer.size(); ++k) {
            p.trailer[k] = rec[36 + k];
        }
        p.mastermind = p.trailer[CaseParticipant::kMastermindOffset] != 0;
        p.clue_type = p.trailer[CaseParticipant::kClueTypeOffset];
        p.rank = p.trailer[CaseParticipant::kRankOffset];
        file.participants.push_back(std::move(p));
        offset += casefile::kParticipantSize;
    }

    file.events.reserve(events);
    for (std::size_t i = 0; i < events; ++i) {
        const uint8_t* rec = data + offset;
        const int conducted_by = static_cast<int>(readU16LE(rec));
        if (conducted_by != casefile::kNone) {  // skip the blank terminator
            CaseEvent e;
            e.participant = conducted_by;
            e.message_id = static_cast<int>(readU16LE(rec + 4));
            e.text = readFixedString(rec + 6, casefile::kNameLength);
            e.target = rec[38];
            e.type = rec[39];
            e.received_mask = rec[40];
            e.destroyed_mask = rec[41];
            e.value = static_cast<int>(readU16LE(rec + 42));
            file.events.push_back(std::move(e));
        }
        offset += casefile::kEventSize;
    }

    // The object region is a fixed four slots; empty slots carry icon 0xFF and
    // are dropped from the result.
    const std::size_t available = (size - fixed) / casefile::kObjectSize;
    const std::size_t slots = available < casefile::kObjectSlots ? available
                                                                 : casefile::kObjectSlots;
    file.objects.reserve(slots);
    for (std::size_t i = 0; i < slots; ++i) {
        const uint8_t* rec = data + offset;
        CaseObject o;
        o.name = readFixedString(rec, casefile::kObjectNameLength);
        o.icon = rec[16];
        if (o.icon != casefile::kNone) {
            file.objects.push_back(std::move(o));
        }
        offset += casefile::kObjectSize;
    }

    out = std::move(file);
    return true;
}


bool loadCaseFile(const std::string& path, CaseFile& out) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        return false;
    }

    const std::streamoff length = in.tellg();
    if (length < 0) {
        return false;
    }

    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<std::size_t>(length));
    if (!buffer.empty()
        && !in.read(reinterpret_cast<char*>(buffer.data()), length)) {
        return false;
    }

    return parseCaseFile(buffer.data(), buffer.size(), out);
}
