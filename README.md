# Event log parser

Example interview question solution (event log parser) with O complexity analysis.

## Usage

The C++ executable needs to be called with log filename as an argument.
It will parse the log and print out all the results for all three required functionalities:
1. IP that received the most distinct user logins
2. Second functionality - user that at one point had the highest number of sessions open
3. Third functionality - average session length per IP

### Example usage for log example given in the exercise:

    $ ./eventLog log_example.txt 

IP that received the most distinct user logins: 10.0.0.2
User that at one point had the highest number of sessions open: user1
Average session length per IP: (10.0.0.1: 300s), (10.0.0.5: 60s), (10.0.0.2: 450s)

### Assumptions:

1. Matching login and logout events of the same user on the same IP: it is assumed that the logout event closes the session of the login event of the corresponding index when events are sorted in time. So the first logout closes the first login and the last logout closes the last login.
2. User field maximum length: it is assumed that the maximum length for user name is 20 characters.
3. Regarding the first functionality (IP that received the most distinct user logins): every login is considered distinct, even if there are multiple logins from the same user on the same IP.

### Approach

Four classes are used:

**Event** - represents an event described by one log line. Has getters and setters for event properties: time, event type, IP address and username. Setters take a string that represents a parameter, validate it and update the event property if validation passes.

**EventLog** - represents a group of events from one log file (uses the Event class). Holds a vector of Events. Has a method for adding a new event and getters for getting the stats for all three functionalities:
 - IP that received the most distinct user logins,
 - User that at one point had the highest number of sessions open,
 - Average session length per IP.
For implementation details of these functionalities, please see below under "Big-O complexity".

**EventBuilder** - builds an Event instance from a given log line string (uses the Event class). It tokenizes the given string into four tokens and for each of them calls the appropriate setter method of the Event class.

**EventLogBuilder** - builds an EventLog instance from a given log file stream (builds the Events using the EventBuilder and adds them to the EventLog).

Main method will:
1) Open a file stream,
2) Build an EventLog from the file stream using the EventLogBuilder,
3) Call the sort method on the EventLog,
4) Print the results of all three required functionalities by getting the stats from the EventLog getter methods.


### Big-O complexity:

#### Parsing the file and building the EventLog instance.
Time and space complexity: O(n) - linear dependence on the number of the events.
   
#### Sorting the Events in the EventLog. This is needed for the functionalities of calculating the user which had the highest number of sessions open and the functionality of calculating the average session length per IP address.

Since the std::sort C++ method is used, the average time complexity is O(n log n) and space complexity is O(log n).

#### First functionality - IP that received the most distinct user logins:
First, the unordered_map with IP as key and number of logins for each IP as value is populated. This is done by iterating through the vector of Events (time complexity O(n)) and updating the unordered_map whenever a login event is found (insertion into unordered_map is O(1)). Then, we iterate through unordered_map to find the max logins value. So, time complexity is O(n)*O(1) + O(n) = O(n).
Space complexity is also O(n) because number of elements in the unordered_map is linearly dependent on number of login events.
  
#### Second functionality - user that at one point had the highest number of sessions open:
First, we iterate (O(n)) through the (already sorted by time) events vector and we build an unordered_map with user as the key and two ints pair as values (currentSessions, maxSessions). For each event in the vector, we update the "current" number of sessions for a user by incrementing (login event) or decrementing (logout event) the currentSessions. If currentSessions for that user is larger than maxSessions, we also update maxSessions. Update of unordered_map element is O(1).
Once this is done, we have an unordered_map of users with their respective maxSessions. Then we simply iterate through it to find the user with max maxSessions (O(n)).
So, the time complexity is O(n)*O(1) + O(n) = O(n).
Space complexity is O(n) because we use an unordered_map for which the number of elements linearly depends on the number of login/logout events.
  
#### Third functionality - average session length per IP:
First, we build two unordered_maps with IP as key. Value in both is another unordered_map with user as key, and with vector of login times (first map) and logout times (second map).
We go through all the events and fill these two unordered_maps with login and logout times per IP and user. The time complexity of this part is O(n) because insertion in unordered_maps and vector is O(1), and we do it n times (number of events). Space complexity is O(n), dependent also on number of events.
Now that we have login and logout times classified by IP and user, we can calculate the average session length for every IP. For each IP, we go through unordered_map with users as keys, and for every user we go through login times from the beginning to the end and through logout times from the beginning to the end. This way, for each user, we match the first logout time with the first login time and so on until the last logout time is matched with the last login time. For every IP session lengths are added and divided by the number of sessions for that IP. That is the average session length for that IP, which is saved.
This part also has time complexity of O(n) because we access each event only once, and the access for unordered_maps is O(1). Vector of login/logout times is accessed via operator[] which also has complexity O(1). Space complexity is O(n) - linear dependence on the number of events.
In total, time and space complexity for the third functionality is O(n).
