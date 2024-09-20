#include <iostream>
#include <cstdio>
#include <arpa/inet.h>
#include <algorithm>
#include <memory.h>
#include "eventLog.h"

using namespace std;

////// Helper methods ///////////////

namespace 
{

// Returns non-zero if given string is a valid IPv4 address
bool isValidIpV4Address(const char *ipAddress)
{
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
  return result != 0;
}

// Checks if a given string is alpha-numberic only
bool isStringAlphaNumeric(const std::string str)
{
  std::string::size_type i = 0;
  while (i < str.length())
  {
    if (!std::isalnum(str[i++]))
    {
      return false;
    }
  }
  
  return true;
}

// Compares two log events by time (used for sorting)
bool compareEventsByTime(Event eventA, Event eventB)
{ 
  struct tm tm1 = eventA.getTime();
  struct tm tm2 = eventB.getTime();
  
  time_t t1 = mktime(&tm1);
  time_t t2 = mktime(&tm2);
  
  double diffSecs = difftime(t1, t2);
  
  return (diffSecs < 0);
}

// Checks if a given time structure can be correctly interpreted
bool validateTime(struct tm timeInfo)
{  
  // If time format is invalid, mktime returns -1
  time_t rawTime = mktime(&timeInfo);
  
  //cout << "mktime returns: " << rawTime << endl;
  
  return (rawTime != -1);
}

// For every IP in the given EventLog, prints the average session length
void printAvgSessionLengthPerIp(const EventLog& eventLog)
{
  EventLog::IpMap avgSessionLengths;
  eventLog.getAvgSessionLengthPerIp(avgSessionLengths);
  
  cout << "Average session length per IP: ";
  bool firstLine = true;
  for (EventLog::IpMap::iterator it = avgSessionLengths.begin(); it != avgSessionLengths.end(); ++it)
  {    
    if (firstLine)
    {
      firstLine = false;
    }
    else
    {
      cout << ", ";
    }
    
    string ip = it->first;
    int length = it->second;
    
    cout << "(" << ip << ": " << length << "s)";
  }
  cout << endl;
}

} // namespace


// Event class public members /////////////////////////

struct tm Event::getTime() const
{
  return eventTime;
}

bool Event::setTime(const std::string timeStr)
{
  int hour, min, sec;
  
  if (sscanf(timeStr.c_str(), "%02d:%02d:%02d", &hour, &min, &sec)
      != 3)
  {
    // Invalid time format
    return false;
  }
  
  struct tm timeInfo = {0};
  timeInfo.tm_hour = hour;
  timeInfo.tm_min = min;
  timeInfo.tm_sec = sec;
    
  if (validateTime(timeInfo))
  {
    eventTime = timeInfo;
    return true;
  }
  else
  {
    return false;
  }
}

Event::EventType Event::getType() const
{
  return eventType;
}

bool Event::setType(const std::string typeStr)
{
  if (typeStr.compare("LOGIN") == 0)
  {
    eventType = Event::EVENT_LOGIN;
  }
  else if (typeStr.compare("LOGOUT") == 0)
  {
    eventType = Event::EVENT_LOGOUT;
  }
  else
  {
    return false;
  }
  
  return true;
}

std::string Event::getIp() const
{
  return ip;
}

bool Event::setIp(const std::string ipStr)
{
  if (isValidIpV4Address(ipStr.c_str()))
  {
    ip = ipStr;
    return true;
  }
  else
  {
    return false;
  }
}

std::string Event::getUser() const
{
  return user;
}

bool Event::setUser(const std::string userStr)
{
  if (isStringAlphaNumeric(userStr))
  {
    user = userStr;
    return true;
  }
  else
  {
    return false;
  }
}

// EventLog class public members ////////////////////////////

void EventLog::addEvent(Event e)
{
  events.push_back(e);
}

void EventLog::sortEvents()
{
  sort(events.begin(), events.end(), compareEventsByTime);
}

string EventLog::getMostDistinctLoginsIp() const
{
  // Calculate the number of distinct logins per IP
  IpMap loginsPerIp;
  for (vector<Event>::const_iterator it = events.begin(); it != events.end(); ++it)
  {
    if (it->getType() == Event::EVENT_LOGIN)
    {
      string ip = it->getIp();
    
      loginsPerIp[ip]++;
    }
  }
  
  // Find and return the IP with most distinct logins
  string mostDistinctLoginsIp;
  int mostDistinctLogins = 0;
  for (IpMap::const_iterator it = loginsPerIp.begin(); it != loginsPerIp.end(); ++it)
  {
    if (it->second > mostDistinctLogins)
    {
      mostDistinctLoginsIp = it->first;
      mostDistinctLogins = it->second;
    }
  }
  
  return mostDistinctLoginsIp;
}

string EventLog::getHighestPeakSessionsUser() const
{
  // Find max sessions for each user
  UserMap userMap;
  for (vector<Event>::const_iterator it = events.begin(); it != events.end(); ++it)
  {
    Event::EventType eventType = it->getType();
    string user = it->getUser();
    
    UserSessionsStats* stats = &userMap[user];
    
    // For every login/logout update the number of sessions open
    if (eventType == Event::EVENT_LOGIN)
    {
      stats->currentSessions++;
    }
    else if (eventType == Event::EVENT_LOGOUT)
    {
      stats->currentSessions--;
    }
    
    // Keep track of highest number of sessions ever open per each user
    if (stats->currentSessions > stats->maxSessions)
    {
      stats->maxSessions = stats->currentSessions;
    }
  }
  
  // Now that we know the max number of sessions for every user,
  // find the user with the highest number
  int maxSessions = 0;
  string userWithMaxSessions;
  for (UserMap::const_iterator it = userMap.begin(); it != userMap.end(); ++it)
  {
    string user = it->first;
    int currMaxSessions = (it->second).maxSessions;
    if (currMaxSessions > maxSessions)
    {
      maxSessions = currMaxSessions;
      userWithMaxSessions = user;
    }
  }
  
  return userWithMaxSessions;
}

void EventLog::getAvgSessionLengthPerIp(IpMap& avgSessionLengths) const
{ 
  IpLogTimesMap loginTimesMap;
  IpLogTimesMap logoutTimesMap;
  
  // Classify each login/logout event by IP and user
  for (vector<Event>::const_iterator it = events.begin(); it != events.end(); ++it)
  {
    string ip = it->getIp();
    string user = it->getUser();
    Event::EventType eventType = it->getType();
    struct tm eventTime = it->getTime();

    if (eventType == Event::EVENT_LOGIN)
    {
      LogTimes* loginTimes = &loginTimesMap[ip][user];
      loginTimes->push_back(eventTime);
    }
    else if (eventType == Event::EVENT_LOGOUT)
    {
      LogTimes* logoutTimes = &logoutTimesMap[ip][user];
      logoutTimes->push_back(eventTime);
    }
  }
  
  // For every IP calculate its average session length and save it
  for (IpLogTimesMap::iterator it = loginTimesMap.begin(); it != loginTimesMap.end(); ++it)
  {
    string ip = it->first;

    UserLogTimesMap& loginTimes = it->second;
    UserLogTimesMap& logoutTimes = logoutTimesMap[ip];
    
    avgSessionLengths[ip] = calcAvgSessionLengthForIp(loginTimes, logoutTimes);
  }
}

// EventLog class private members ////////////////////////////

int EventLog::calcAvgSessionLengthForIp(UserLogTimesMap& loginTimesMap,
                                        UserLogTimesMap& logoutTimesMap) const
{
  int lengthSum = 0;
  int sessions = 0;

  // Calculate session lengths for every user
  for (UserLogTimesMap::iterator it = loginTimesMap.begin(); it != loginTimesMap.end(); ++it)
  {
    string user = it->first;
    LogTimes& loginTimes = it->second;
    LogTimes& logoutTimes = logoutTimesMap[user];
    
    // For every login and logout pair for this user, calculate the session length
    unsigned int i = 0;
    while (i < loginTimes.size() && i < logoutTimes.size())
    {        
      time_t t1 = mktime(&loginTimes[i]);
      time_t t2 = mktime(&logoutTimes[i]);
      
      double sessionLength = difftime(t2, t1);
      
      // Update total number of sessions and add length to total session length sum
      sessions++;
      lengthSum += sessionLength;
      
      i++;
    }
  }
  
  if (sessions == 0)
  {
    return -1;
  }
  
  // return the average session length
  return lengthSum / sessions;
}

// EventBuilder class /////////////////////

EventBuilder::EventBuilder(string eventLogLine) : isEventValid(true)
{
  Event newEvent;
  EventLogLineTokens tokens;  
  
  if (!tokenizeLogLine(eventLogLine, tokens))
  {
    // Ignore invalid format or other event (non-login/logout)
    isEventValid = false;
  }
  
  if (!newEvent.setTime(tokens.timeStr))
  {
    // Invalid time format
    isEventValid = false;
  }
  
  if (!newEvent.setType(tokens.typeStr))
  {
    // Invalid event type or non-login/logout event
    isEventValid = false;
  }
  
  if (!newEvent.setIp(tokens.ip))
  {
    // Invalid IPv4 format
    isEventValid = false;
  }

  if (!newEvent.setUser(tokens.user))
  {
    // Invalid user format
    isEventValid = false;
  }
  
  if (isEventValid)
  {
    event = newEvent;
  }
}

bool EventBuilder::getEvent(Event& e) const
{
  if (isEventValid)
  {
    e = event;
    return true;
  }
  else
  {
    return false;
  }
}

bool EventBuilder::tokenizeLogLine(std::string logLine,
       EventLogLineTokens& tokens) const
{
  const static int TimeStrLen = 8 + 1;  // hh:mm:ss
  const static int TypeStrLen = 6 + 1;  // LOGIN or LOGOUT
  const static int IpStrLen   = 15 + 1; // xxx.xxx.xxx.xxx
  const static int UserStrLen = 20 + 1; // assumption that user field is max 20 chars
  
  char timeChar[TimeStrLen];
  char typeChar[TypeStrLen];
  char ipChar[IpStrLen];
  char userChar[UserStrLen];
  
  if (sscanf(logLine.c_str(), "%[^,], %[^,], %[^,], %s",
      timeChar, typeChar, ipChar, userChar) != 4)
  {
    // Invalid format
    return false;
  };
  
  tokens.timeStr = timeChar;
  tokens.typeStr = typeChar;
  tokens.ip = ipChar;
  tokens.user = userChar;
  
  return true;
}

// EventLogBuilder class public members ///////////////

EventLogBuilder::EventLogBuilder(ifstream& logFile, EventLog& eventLog)
{
  string logLine;
  while (getline(logFile, logLine))
  {
    // Make an Event from log line
    EventBuilder eventBuilder(logLine);
    Event event;
    
    // If the event is valid, save it to the EventLog
    if (eventBuilder.getEvent(event))
    {
      eventLog.addEvent(event);
    }
  }
}

// Main method:
// Use the EventLogBuilder to build the EventLog from a given log file
// Print the stats from the EventLog

int main (int argc, char *argv[])
{    
  if (argc != 2)
  {
      cout << "usage: " << argv[0] << " <filename>" << endl;
  }
  else
  {
    ifstream logFile(argv[1]);
    if (!logFile.is_open())
    {
      cout << "Could not open file" << endl;
    }
    else
    {
      // Build the EventLog by parsing the file
      EventLog eventLog;
      EventLogBuilder eventLogBuilder(logFile, eventLog);
      
      // Sort the EventLog
      eventLog.sortEvents();
      
      // Print the stats:      
      // Rresult of the first functionality
      cout << "IP that received the most distinct user logins: "
           << eventLog.getMostDistinctLoginsIp() << endl;
      
      // Result of the second functionality
      cout << "User that at one point had the highest number of sessions open: "
           << eventLog.getHighestPeakSessionsUser() << endl;
      
      // Result of the third functionality
      printAvgSessionLengthPerIp(eventLog);
    }
  }
}
