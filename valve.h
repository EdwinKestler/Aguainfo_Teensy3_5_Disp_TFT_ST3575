class Valve
{
  // Class Member Variables
  // These are initialized at startup
  int ValvePin;      // the number of the Valve pin
 
  // These maintain the current state
  int ValveState;                 // ledState used to set the LED

 
  // Constructor - creates a Flasher 
  // and initializes the member variables and state
  public:
  Valve(int pin)
  {
  ValvePin = pin;
  pinMode(ValvePin, OUTPUT);     
  ValveState = LOW; 
  }
 
  void Update()
  {
    if(ValveState == HIGH)
    {
      ValveState = LOW;  // Turn it off
      digitalWrite(ValvePin, ValveState);  // Update the actual Valve
      Serial.print(F("OFF"));
      Serial.print(F(""));
    }
    else if (ValveState == LOW)
    {
      ValveState = HIGH;  // turn it on
      digitalWrite(ValvePin, ValveState);   // Update the actual Valve
      Serial.print(F("ON"));
      Serial.print(F(""));
    }
  }
};
