#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "../types.h"  // City (the single canonical definition)
#include "casefile.h"  // CaseFile (parsed CRIME*.DTA templates)

#define NUM_MISSION_TYPES 13

int case_gen();


struct Organization {
  std::string full_name;
  std::string abrv;
  char id_char;  // TODO: type
};

// `inline` gives these tables a single, shared definition while keeping them in
// the header. They used to be plain globals, so a second translation unit that
// included this header would have failed to link.
inline const Organization ownOrg { "CIA", "CIA", '?' };

inline const std::array<Organization, 3> alliedOrg {
  Organization{"KGB",    "KGB",   '!'},
  Organization{"MI6",    "MI6",   '@'},
  Organization{"Mossad", "Mossd", '#'},
};


enum {
  AMAZON_CARTEL    = 'a',  // TODO: better this way?
  COLUMBIAN_CARTEL = 'b',
};

inline const std::array<Organization, 26> crimeOrg {
  Organization{"Amazon Cartel",        "?",     'a'},  // TODO: assign the correct letters and abbrevs
  Organization{"Colombian Cartel",     "?",     COLUMBIAN_CARTEL},
  Organization{"Death Squads",         "?",     'c'},
  Organization{"Dignity Battalion",    "?",     'd'},
  Organization{"Direct Action",        "DActn", 'e'},
  Organization{"FLN",                  "?",     'f'},
  Organization{"Haitian Junta",        "?",     'g'},
  Organization{"IraqiSP",              "IrqSP", 'h'},
  Organization{"Jamaican Gang",        "?",     'i'},
  Organization{"Libyan Embassy",       "LibEb", 'j'},
  Organization{"M18",                  "M18",   'k'},
  Organization{"Mafia",                "Mafia", 'l'},
  Organization{"Marxists",             "Mrxst", 'm'},
  Organization{"Mercenaries",          "?",     'n'},
  Organization{"Muslim Jihad",         "?",     'o'},
  Organization{"PIFA",                 "PIFA",  'p'},
  Organization{"PFO",                  "PFO",   'q'},
  Organization{"PRC",                  "PRC",   'r'},
  Organization{"Red Army",             "RArmy", 's'},
  Organization{"Red Battalion",        "RedBt", 't'},
  Organization{"Red September",        "?",     'u'},
  Organization{"Revolutionary Guards", "?",     'v'},
  Organization{"Shining Way",          "?",     'w'},
  Organization{"Stassi",               "Stasi", 'x'},
  Organization{"Tupamaros",            "?",     'y'},
  Organization{"UnionCorsique",        "Union", 'z'},
};


std::string generateName(Organization org);


enum class Rank : uint16_t {
  Nobody       =   0,
  Operative    =  20,
  Technician   =  25,
  Agent        =  30,
  Organizer    =  35,
  SpecialAgent =  40,
  GroupLeader  =  45,
  Mastermind   = 200,
};


enum class Status {
  AtLarge,
  RedHerring,
  DoubleAgent,
  Arrested,
  InHiding,
};


class Participant {
public:
  explicit Participant(std::string role);

  std::string getName() const        { return name; }
  char        getAgentLetter() const { return agentletter; }
  uint16_t    getRankPoints() const  { return static_cast<uint16_t>(rank); }
  std::string getRole() const        { return role; }

private:
  std::string  name;
  char         agentletter { 'Q' };  // TODO: pickRandomUnusedLetter()
  Rank         rank { Rank::Operative };
  City         city {};
  Organization affiliation {};
  City         recruitcity {};
  Participant* p_recruiter { nullptr };  // TODO: link recruiters after all participants exist
  std::string  role;
  Status       status { Status::AtLarge };
  uint8_t      visibility { 0x40 };
};


class Mission {
public:
  explicit Mission(std::string name);

  void addParticipant(std::string role);
  void printOverview() const;

private:
  std::string name;
  std::vector<Participant> participants;
  /* TODO */
};


class MissionSet {
public:
  MissionSet(Mission, Mission, Mission);  // TODO: direct init the three missions
};


extern std::vector<Mission> missions;  // TODO: switch to a set type with name as key

// The crime templates loaded from the original CRIME*.DTA files, in crime
// number order. Empty until case_gen() runs (or if the data files are absent).
extern std::vector<CaseFile> caseFiles;
