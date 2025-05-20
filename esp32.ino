#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_eth.h>
#include <HTTPClient.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
// Function to URL encode a string for the Wolfram API
String urlEncode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isAlphaNumeric(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

// WiFi credentials - replace with your network details
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Derivative API settings
const String derivativeBaseURL = "https://newton.vercel.app/api/v2/integrate/";

// LCD setup
LiquidCrystal lcd(15, 2, 21, 19, 18, 5);

// Extra buttons on digital pins - repurpose for direct operations
const int extraButtons[] = {22, 23, 16, 17};
const int numExtraButtons = 4;
String extraButtonFunctions[] = {"+", "-", "*", "/"};  // Basic operations using standard ASCII

// Keypad setup - Casio-style layout
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'7', '8', '9', 'D'},  // D = DEL/AC
  {'4', '5', '6', 'S'},  // S = SHIFT
  {'1', '2', '3', '='},  // = = Equals/Solve
  {'0', '.', 'E', 'M'}   // E = EXP, M = Mode
};

byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Function definitions for shifted keys
struct FunctionMapping {
  char key;
  char display[12];  // Increased size for longer function names
  char apiSyntax[12]; // Wolfram Alpha API syntax
  byte mode;  // Different function sets
};

// Mode definitions
#define MODE_NORMAL 0
#define MODE_TRIG 1
#define MODE_HYP 2
#define MODE_INV 3
#define MODE_LOG 4
#define NUM_MODES 5

// Function tables for shifted keys
FunctionMapping shiftedFunctions[] = {
  // Shifted functions for Mode 0 (Normal)
  {'7', "x^2", "x^2", 0},
  {'8', "x^n", "x^", 0},
  {'9', "sqrt", "sqrt", 0},
  {'4', "(", "(", 0},
  {'5', ")", ")", 0},
  {'6', "integrate", "integrate", 0},
  {'1', "x", "x", 0},
  {'2', "dx", "dx", 0},
  {'3', "=", "=", 0},
  {'0', "e^x", "e^", 0},
  {'.', "pi", "pi", 0},
  {'E', "ANS", "", 0},
  
  // Mode 1 - Trigonometric
  {'7', "sin", "sin", 1},
  {'8', "cos", "cos", 1},
  {'9', "tan", "tan", 1},
  {'4', "sec", "sec", 1},
  {'5', "csc", "csc", 1},
  {'6', "cot", "cot", 1},
  {'1', "arcsin", "arcsin", 1},
  {'2', "arccos", "arccos", 1},
  {'3', "arctan", "arctan", 1},
  
  // Mode 2 - Hyperbolic
  {'7', "sinh", "sinh", 2},
  {'8', "cosh", "cosh", 2},
  {'9', "tanh", "tanh", 2},
  {'4', "sech", "sech", 2},
  {'5', "csch", "csch", 2},
  {'6', "coth", "coth", 2},
  {'1', "arcsinh", "arcsinh", 2},
  {'2', "arccosh", "arccosh", 2},
  {'3', "arctanh", "arctanh", 2},
  
  // Mode 3 - Inverse functions (additional inverse functions)
  {'7', "arcsec", "arcsec", 3},
  {'8', "arccsc", "arccsc", 3},
  {'9', "arccot", "arccot", 3},
  {'4', "arcsech", "arcsech", 3},
  {'5', "arccsch", "arccsch", 3},
  {'6', "arccoth", "arccoth", 3},
  
  // Mode 4 - Logarithmic and special functions
  {'7', "ln", "ln", 4},
  {'8', "log", "log", 4},
  {'9', "abs", "abs", 4},
  {'4', "Heaviside", "UnitStep", 4},   // Step function
  {'5', "DiracDelta", "DiracDelta", 4},   // Dirac delta
  {'6', "factorial", "!", 4},     // Factorial
  {'1', "e", "e", 4},
  {'2', "10^", "10^", 4},
  {'3', "1/x", "1/", 4}
};

// Display variables
String inputExpression = "";
String apiExpression = "";  // Expression formatted for Wolfram Alpha API
byte currentMode = MODE_NORMAL;
String modeNames[] = {"Normal", "Trig", "Hyper", "Inv", "Log"};
int cursorPos = 0;
bool solving = false;
String result = "";
bool shiftPressed = false;
String lastAnswer = "0";
bool wifiConnected = false;

// Add these variables at the top with other global variables
int resultScrollPosition = 0;
unsigned long lastScrollTime = 0;
const int SCROLL_DELAY = 1000; // 1 second between scrolls

// Add these pins for scroll buttons
const int SCROLL_LEFT_PIN = 4;   // Changed to GPIO 4
const int SCROLL_RIGHT_PIN = 35; // Keep GPIO 35

// Add these variables at the top with other global variables
int inputScrollPosition = 0;
unsigned long lastButtonPress = 0;
const int BUTTON_DEBOUNCE = 200; // 200ms debounce time

// Forward declarations
void updateDisplay();
void addToExpression(String text, String apiText = "");
void deleteChar();
void clearAll();
void changeMode();
void solveWithWolframAlpha();
void processExtraButton(int buttonIndex);
void processShiftedKey(char key);
void connectToWifi();
String queryDerivativeAPI(String query);

void setup() {
  // Initialize LCD
  lcd.begin(16, 2);
  
  // Setup extra buttons as inputs with pull-up resistors
  for (int i = 0; i < numExtraButtons; i++) {
    pinMode(extraButtons[i], INPUT_PULLUP);
  }
  
  // Setup scroll buttons
  pinMode(SCROLL_LEFT_PIN, INPUT_PULLUP);
  pinMode(SCROLL_RIGHT_PIN, INPUT_PULLUP);
  
  Serial.begin(115200);
  
  // Initial display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Integration Calc");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi..");
  
  // Connect to WiFi
  connectToWifi();
  
  updateDisplay();
}

void connectToWifi() {
  WiFi.begin(ssid, password);
  
  // Wait for connection with timeout
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
    lcd.setCursor(15, 1);
    lcd.print(attempts % 2 == 0 ? "." : " ");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Using offline mode");
    delay(2000);
  }
}

void updateDisplay() {
  lcd.clear();
  
  // First line: Mode/Shift status and input
  lcd.setCursor(0, 0);
  if (shiftPressed) {
    lcd.print("S:");
  } else {
    lcd.print(modeNames[currentMode].substring(0, 2) + ":");
  }
  
  // Display current expression (with scrolling if needed)
  if (inputExpression.length() > 13) {
    // Update scroll position for input
    if (inputScrollPosition > inputExpression.length() - 13) {
      inputScrollPosition = inputExpression.length() - 13;
    }
    if (inputScrollPosition < 0) inputScrollPosition = 0;
    
    lcd.setCursor(3, 0);
    lcd.print(inputExpression.substring(inputScrollPosition, inputScrollPosition + 13));
  } else {
    lcd.setCursor(3, 0);
    lcd.print(inputExpression);
  }
  
  // Second line: Result or function hints
  lcd.setCursor(0, 1);
  if (solving) {
    lcd.print("= ");
    
    // Handle scrolling for long results
    if (result.length() > 14) {
      // Update scroll position every SCROLL_DELAY milliseconds
      if (millis() - lastScrollTime >= SCROLL_DELAY) {
        resultScrollPosition++;
        if (resultScrollPosition > result.length() - 14) {
          resultScrollPosition = 0;
        }
        lastScrollTime = millis();
      }
      lcd.print(result.substring(resultScrollPosition, resultScrollPosition + 14));
    } else {
      lcd.print(result);
    }
  } else if (shiftPressed) {
    // Display function key hints depending on mode
    lcd.print("Fn: ");
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < sizeof(shiftedFunctions)/sizeof(shiftedFunctions[0]); j++) {
        if (shiftedFunctions[j].key == '7' + i && shiftedFunctions[j].mode == currentMode) {
          lcd.print(shiftedFunctions[j].key);
          break;
        }
      }
    }
    lcd.print("..");
  } else {
    if (wifiConnected) {
      lcd.print("W+ ");
    } else {
      lcd.print("W- ");
    }
    lcd.print("S=Fn M=mode");
  }
}

void addToExpression(String text, String apiText) {
  inputExpression += text;
  
  // If no specific API text provided, use the display text
  if (apiText.length() == 0) {
    apiExpression += text;
  } else {
    apiExpression += apiText;
  }
  
  cursorPos = inputExpression.length();
  updateDisplay();
}

void deleteChar() {
  if (inputExpression.length() > 0) {
    inputExpression.remove(inputExpression.length() - 1);
    apiExpression.remove(apiExpression.length() - 1);
    cursorPos = inputExpression.length();
  }
  updateDisplay();
}

void clearAll() {
  inputExpression = "";
  apiExpression = "";
  cursorPos = 0;
  solving = false;
  updateDisplay();
}

void changeMode() {
  currentMode = (currentMode + 1) % NUM_MODES;
  shiftPressed = false;  // Reset shift state on mode change
  updateDisplay();
}

void toggleShift() {
  shiftPressed = !shiftPressed;
  updateDisplay();
}

void getFunctionForShiftedKey(char key, String &displayText, String &apiText) {
  // Initialize with empty strings instead of key
  displayText = "";
  apiText = "";
  
  // Find the matching function
  for (int i = 0; i < sizeof(shiftedFunctions)/sizeof(shiftedFunctions[0]); i++) {
    if (shiftedFunctions[i].key == key && shiftedFunctions[i].mode == currentMode) {
      displayText = String(shiftedFunctions[i].display);
      apiText = String(shiftedFunctions[i].apiSyntax);
      return;
    }
  }
  
  // If no function found, use the key as is
  displayText = String(key);
  apiText = String(key);
}

void solveWithWolframAlpha() {
  solving = true;
  result = "Computing...";
  resultScrollPosition = 0;  // Reset scroll position
  updateDisplay();
  
  // Format the query for integration
  String mathQuery = apiExpression;
  
  // Clean up the expression
  mathQuery.replace(" ", "");
  
  Serial.print("Sending to Derivative API: ");
  Serial.println(mathQuery);
  
  if (wifiConnected) {
    // Send to Derivative API
    String apiResult = queryDerivativeAPI(mathQuery);
    
    if (apiResult.length() > 0) {
      result = apiResult;
      // Clean up the result
      result.trim();
      
      // Format the result for display
      result.replace(" + ", "+");
      result.replace(" - ", "-");
      result.replace(" * ", "*");
      result.replace(" / ", "/");
      result.replace("sin(", "sin");
      result.replace("cos(", "cos");
      result.replace("tan(", "tan");
      result.replace(")", "");
      result.replace("constant", "C");
      result.replace("integral", "∫");
      
      // Remove any remaining spaces
      result.replace(" ", "");
      
      // If result is empty or just contains C, add the constant
      if (result.length() == 0 || result == "C") {
        result = "C";
      }
      
      lastAnswer = result;
    } else {
      result = "API Error";
    }
  } else {
    result = "No WiFi";
    delay(1000);
    result = "Connect WiFi!";
  }
  
  updateDisplay();
}

String queryDerivativeAPI(String query) {
  HTTPClient http;
  String url = derivativeBaseURL + query;
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  http.begin(url);
  int httpResponseCode = http.GET();
  
  String response = "";
  
  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
    
    // Parse the JSON response
    int startPos = response.indexOf("\"result\":\"");
    if (startPos >= 0) {
      startPos += 10;  // Length of "result":"
      int endPos = response.indexOf("\"", startPos);
      if (endPos >= 0) {
        response = response.substring(startPos, endPos);
      }
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    response = "Error " + String(httpResponseCode);
  }
  
  http.end();
  return response;
}

void processExtraButton(int buttonIndex) {
  String ops[] = {"+", "-", "*", "/"};
  addToExpression(ops[buttonIndex], ops[buttonIndex]);
  shiftPressed = false;  // Reset shift after operation
}

void processShiftedKey(char key) {
  String displayFunction, apiFunction;
  getFunctionForShiftedKey(key, displayFunction, apiFunction);
  
  // Only process if we got a valid function
  if (displayFunction.length() > 0) {
    // Special cases for certain functions
    if (displayFunction == "integrate") {
      addToExpression("∫(", "integrate(");
    } else if (displayFunction == "ANS") {
      addToExpression(lastAnswer, lastAnswer);
    } else {
      if (displayFunction == "sin" || displayFunction == "cos" || displayFunction == "tan" || 
          displayFunction == "sec" || displayFunction == "csc" || displayFunction == "cot" ||
          displayFunction == "sinh" || displayFunction == "cosh" || displayFunction == "tanh" ||
          displayFunction == "ln" || displayFunction == "log" || displayFunction == "abs" ||
          displayFunction.indexOf("arc") >= 0) {
        // Functions that need parentheses
        addToExpression(displayFunction + "(", apiFunction + "(");
      } else {
        // Regular functions
        addToExpression(displayFunction, apiFunction);
      }
    }
  }
  
  shiftPressed = false;  // Reset shift state after use
}

void loop() {
  // Check keypad
  char key = keypad.getKey();
  
  if (key) {
    if (key == 'S') {
      toggleShift();
    } else if (key == 'M') {
      changeMode();
    } else if (key == 'D') {
      if (shiftPressed) {
        clearAll();  // AC function when shifted
      } else {
        deleteChar();  // DEL function when not shifted
      }
      shiftPressed = false;
    } else if (key == '=') {
      solveWithWolframAlpha();
      shiftPressed = false;
    } else if (shiftPressed) {
      processShiftedKey(key);
    } else {
      // Normal keys - numbers, decimal point, etc.
      addToExpression(String(key), String(key));
    }
  }
  
  // Check extra buttons for basic operations
  for (int i = 0; i < numExtraButtons; i++) {
    if (digitalRead(extraButtons[i]) == LOW) {
      processExtraButton(i);
      delay(200);  // Simple debounce
    }
  }
  
  // Check scroll buttons with debounce
  if (millis() - lastButtonPress > BUTTON_DEBOUNCE) {
    if (digitalRead(SCROLL_LEFT_PIN) == LOW) {
      if (solving && result.length() > 14) {
        resultScrollPosition = (resultScrollPosition - 1 + result.length() - 14) % (result.length() - 14);
      } else if (inputExpression.length() > 13) {
        inputScrollPosition = max(0, inputScrollPosition - 1);
      }
      updateDisplay();
      lastButtonPress = millis();
    }
    else if (digitalRead(SCROLL_RIGHT_PIN) == LOW) {
      if (solving && result.length() > 14) {
        resultScrollPosition = (resultScrollPosition + 1) % (result.length() - 14);
      } else if (inputExpression.length() > 13) {
        inputScrollPosition = min((int)(inputExpression.length() - 13), inputScrollPosition + 1);
      }
      updateDisplay();
      lastButtonPress = millis();
    }
  }
  
  // Monitor WiFi connection
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    updateDisplay();
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    updateDisplay();
  }
}