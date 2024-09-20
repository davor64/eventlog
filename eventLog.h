#ifndef EVENTLOG_H
#define EVENTLOG_H

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <ctime>

// Holds the info about the event and provides methods for
// getting and setting the event parameters

class Event
{
public:
  enum EventType
  {
      EVENT_LOGIN = 0,
      EVENT_LOGOUT,
      EVENT_INVALID
  };
  
  struct tm getTime() const;
  bool setTime(const std::string timeStr);
  
  EventType getType() const;
  bool setType(const std::string typeStr);
  
  std::string getIp() const;
  bool setIp(const std::string ipStr);
  
  std::string getUser() const;
  bool setUser(const std::string userStr);
  
private:
  struct tm eventTime;
  EventType eventType;
  std::string ip;
  std::string user;
};

// Holds the list of the log events and provides methods for getting
// various statistics about the events

class EventLog
{
public:
  // Map of IPs and integers, used in several methods of this class
  typedef std::unordered_map<std::string, int> IpMap;
  
  // Functionality 1: get the IP which had the most distinct
  //                  logins in total
  std::string getMostDistinctLoginsIp() const;

  // Functionality 2: get the user with highest peak sessions
  //                  open at one point in time
  struct UserSessionsStats
  {
    int currentSessions;
    int maxSessions;
  };
  typedef std::unordered_map<std::string, UserSessionsStats> UserMap;
  std::string getHighestPeakSessionsUser() const;
  
  // Functionality 3: get the list of average session lenghts for every IP
  typedef std::vector<struct tm> LogTimes;
  typedef std::unordered_map<std::string, LogTimes> UserLogTimesMap;
  typedef std::unordered_map<std::string, UserLogTimesMap> IpLogTimesMap;
  void getAvgSessionLengthPerIp(IpMap& avgSessionLengths) const;

  // Add event to the EventLog
  void addEvent(Event e);

  // Sort events by time
  void sortEvents();
  
private:
  // List of events
  std::vector<Event> events;
  
  // Calculate average session length for a given list of
  // users and their sessions
  int calcAvgSessionLengthForIp(UserLogTimesMap& loginTimesMap,
                                UserLogTimesMap& logoutTimesMap) const;
  

};

// Builds the Event from the event log string

class EventBuilder
{
public:
  EventBuilder(std::string eventLogLine);
  bool getEvent(Event& event) const;
  
private:
  Event event;
  bool isEventValid;
  
  // Break down an event log line into strings for time, type, IP and user
  struct EventLogLineTokens
  {
    std::string timeStr;
    std::string typeStr;
    std::string ip;
    std::string user;
  };
  bool tokenizeLogLine(std::string logLine, EventLogLineTokens& tokens) const;
};

// Builds the EventLog from file stream

class EventLogBuilder
{
public:
  // Make an Event from the log line string and add it to the EventLog
  EventLogBuilder(std::ifstream& logFile, EventLog& eventLog);
};

#endif /* EVENTLOG_H */
