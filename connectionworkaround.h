#define SSID   "Georgian" //  "Georgian" //  "Rogers56372-5G" // Network name
#define PASS   "" //  "" //  "brent1234" //WiFi pass

#define TIMEOUT     10000 // mS used only in echoFind function
#define CONTINUE    false // halt on fault

//part of the code responsible for WiFi connection and getting HTTP response, taken from Ross Bigelow code

unsigned int long deadline; // time interval for reading Serial1 (modem) and parsing the output  

// Print error message and loop stop.
void errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("HALT");
  while (true) {};
}


// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();

  //   Fail if the target string has not been sent by deadline.
  deadline = millis() + TIMEOUT;
  while (millis() < deadline)
  {
    if (Serial1.available())
    {
      char ch = Serial1.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

// Echo module output until 3 newlines encountered.
// (Used when we're indifferent to "OK" vs. "no change" responses.)
void echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}

// Send a command to the module and wait for acknowledgement string
// (or flush module output if no ack specified).
// Echoes all data received to the serial monitor.
boolean echoCommand(String cmd, String ack, boolean halt_on_fail)
{
  Serial1.println(cmd);
#ifdef ECHO_COMMANDS
  Serial.print("--"); Serial.println(cmd);
#endif

  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string
      if (halt_on_fail)
        errorHalt(cmd + " failed"); // Critical failure halt.
      else
        return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}


boolean connectWiFi() // connecting to the wifi using credentials 
{
  String cmd = "AT+CWJAP=\""; cmd += SSID; cmd += "\",\""; cmd += PASS; cmd += "\"";
  if (echoCommand(cmd, "OK", CONTINUE)) // Join Access Point
  {
    Serial.println("Connected to WiFi.");
    return true;
  }
  else
  {
    Serial.println("Connection to WiFi failed.");
    return false;
  }
}
