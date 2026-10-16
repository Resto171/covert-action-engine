#include "casegenerator.h"

#include <iostream>
#include <string>
#include <utility>


std::vector<Mission> missions;
std::vector<CaseFile> caseFiles;


std::string generateName(Organization org) {
  switch (org.id_char) {
    case COLUMBIAN_CARTEL:
      return std::string("Anita Todo");
    default:
      return std::string("Mark Todo");  // TODO: real name generation
  }
}


Participant::Participant(std::string role)
  : name{},
    agentletter{ 'Q' },
    rank{ Rank::Operative },
    city{},
    affiliation{},          // leave unassigned for now, but *initialized*
    recruitcity{},
    p_recruiter{ nullptr },
    role{ std::move(role) },
    status{ Status::AtLarge },
    visibility{ 0x40 }
{
  // NOTE: generateName() reads affiliation, so it must run after the member is
  // initialized (it used to be called in the body before affiliation was set).
  name = generateName(affiliation);
}


Mission::Mission(std::string name)
  : name{ std::move(name) }
{}


void Mission::addParticipant(std::string role) {
  participants.emplace_back(std::move(role));
}


void Mission::printOverview() const {
  std::cout << name << '\n';
  std::cout << std::string(name.length(), '=') << '\n';
  for (const auto& party : participants) {
    std::cout << party.getName() << ", " << party.getRole() << '\n';
  }
}


MissionSet::MissionSet(Mission, Mission, Mission) {
  // TODO: direct init the three missions of the set
}


namespace {

// Directories that may hold the original CRIME*.DTA files, tried in order: the
// working directory first, then the documented data locations.
const char* const kCrimeDirs[] = {
    "",
    "data/Sid Meier's Covert Action/MPS/COVERT/",
    "MPS/COVERT/",
    "COVERT/",
};

std::string crimeFileName(int crime) {
    return "CRIME" + std::to_string(crime) + ".DTA";
}

}  // namespace


// Load every crime template the data files provide. This replaces the old
// placeholder that read a hand-written mission.dat. Missing data is not fatal:
// the caller simply gets an empty list.
int case_gen() {
    caseFiles.clear();
    caseFiles.reserve(NUM_MISSION_TYPES);

    for (int crime = 0; crime < NUM_MISSION_TYPES; ++crime) {
        CaseFile file;
        for (const char* dir : kCrimeDirs) {
            if (loadCaseFile(std::string(dir) + crimeFileName(crime), file)) {
                caseFiles.push_back(std::move(file));
                break;
            }
        }
    }

    if (caseFiles.empty()) {
        std::cerr << "case_gen: no CRIME*.DTA files found\n";
        return 1;
    }

    for (const CaseFile& file : caseFiles) {
        std::cout << "Crime template: " << file.participants.size() << " participants, "
                  << file.events.size() << " events, " << file.objects.size() << " objects\n";
    }
    return 0;
}
