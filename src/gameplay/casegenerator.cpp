#include "casegenerator.h"

#include <fstream>
#include <iostream>
#include <utility>


std::vector<Mission> missions;


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


int case_gen() {  // TODO-debug
  missions.reserve(NUM_MISSION_TYPES);

  std::ifstream infile{"mission.dat"};
  if (!infile) {
    // Fail loudly rather than silently producing an empty case list.
    std::cerr << "case_gen: could not open mission.dat\n";
    return 1;
  }

  std::string s;
  while (std::getline(infile, s)) {
    if (s.empty()) {
      break;
    }
    if (s.starts_with("# ")) {
      missions.emplace_back(s.substr(2));
    } else if (s.starts_with("- ") && !missions.empty()) {
      missions.back().addParticipant(s.substr(2));
    }
  }

  for (const auto& mission : missions) {
    mission.printOverview();
  }
  return 0;
}
