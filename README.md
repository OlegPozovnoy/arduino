# arduino
Arduino final project, 28.04.2018 (C/C++)

Remote cat feeder. 
User makes commands to the tool by changing the page header of the dedicated website arduino71721.azurewebsites.net, for example through FileZilla.
The feeder, when is turned on, connects to google.com to get current timestamp. 
Then every minute it checks the header of the website arduino71721.azurewebsites.net to get and process a new command.

Possible commands are:
NOW - feed now
INSERT HH:MM - add a timestamp to the schedule
DELETE HH:MM - delete a timestamp from the schedule
STATUS - get the current schedule

The output is performed to a dedicated Twitter account Arduino71721 through arduino-tweet.appspot.com API.
 

