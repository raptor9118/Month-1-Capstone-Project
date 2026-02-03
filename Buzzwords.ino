// Crafting Table Academy - Spring 2026
// Month 1 Capstone Project
/* Buzzword Game
   Fast-paced multi-team game where during each round, one player on a team reads clues, 
   while their teammates tries to guess words or phrases containing the "buzzword".  Each 
   round consists of 10 words that the team must guess within 60 seconds.  The person reading
   the clues will press the "Correct" (middle) button for each correct answer, and the "Wrong" 
   (right) button for each wrong answer or for words that are passed.  The "Start Game" (left) 
   button is used to reset the timer and starts the next round.
*/
/* The game currently uses an array to store the base words that are hard-coded, so that I can 
   pick known "easy" words.  Future revisions may ask ChatGPT to provide the base words. 
*/

//Libraries
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define SCREEN_ROTATION 1

// Constants
const unsigned long maxTime = 60;   // Time allowed per round
const int startButton = 2;          // Pin on ESP32 for start button
const int correctButton = 15;       // Pin on ESP32 for correct button
const int wrongButton = 13;         // Pin on ESP32 for wrong button

const char* ssid = "ssid";
const char* password = "password";
const char* OPENAI_API_KEY = "API_KEY";
const char* OPENAI_HOST = "api.openai.com";
const int OPENAI_PORT = 443;
const unsigned long DEBOUNCE_DELAY = 60;  // 60 milliseconds

volatile unsigned long remainingTime = 0;   // Remaining time in the round
volatile unsigned long startTime = 0;       // Start time for the round
volatile unsigned long elapsedSeconds = 0;  // Elapsed time in the round
volatile unsigned long lastPressedTimeS = 0;  // Last time start button was pressed, used for debouncing
volatile unsigned long lastPressedTimeC = 0;  // Last time correct button was pressed, used for debouncing
volatile unsigned long lastPressedTimeW = 0;  // Last time wrong button was pressed, used for debouncing
volatile bool startButtonPressed = false;     // state of start button
volatile bool correctButtonPressed = false;   // state of correct button
volatile bool wrongButtonPressed = false;     // state of wrong button
volatile bool isTimerRunning = false;       // Countdown timer is running, round is in play

int score;      // score, increment by 1 for each correct answer
int i;          // index to answer/question array
int j=-1;       // index to word list, set to -1 to start
String buzzword[5] = {"star", "table", "ball", "stop", "pot"};  // Buzzwords for ChatGPT to generate the questions
String QA[10][2]; // 2-D array for words and questions

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);

  // Initialize variables;
  i = 0;
  score = 0;
  remainingTime = maxTime;

  // Setup Buttons
  pinMode(startButton, INPUT_PULLUP);
  pinMode(correctButton, INPUT_PULLUP);
  pinMode(wrongButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(startButton), handleStartButton, RISING);
  attachInterrupt(digitalPinToInterrupt(correctButton), handleCorrectButton, RISING);
  attachInterrupt(digitalPinToInterrupt(wrongButton), handleWrongButton, RISING);

  // Setup WiFi connection
  setupWiFi();

  // Set up the screen
  tft.init();
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  updateScreen(score, i, remainingTime, -1);
}

void loop() {
  // When start (left) button is pressed, reset score and timer, call ChatGPT to
  // get a list of words and corresponding questions, do a 3 second count down, 
  // then start the game.
  if (startButtonPressed) {
    remainingTime = maxTime;
    score = 0;
    i = 0;
    startButtonPressed = false;
    // Get new questions
    // j is index to buzzword list. Cycles through the buzzwords in the array.
    j += 1;
    int numWords = sizeof(buzzword) / sizeof(buzzword[0]);
    if (j == numWords) {
      j = 0;
    }
    writeScreen(buzzword[j],"Generating words and clues...");
    String txt = getQuestions(buzzword[j]);
    //Serial.println(txt);
    writeScreen(buzzword[j],"Resetting Timer and Score / Starting Game in 3...");
    delay(1000);
    writeScreen(buzzword[j],"Resetting Timer and Score / Starting Game in 2...");
    delay(1000);
    writeScreen(buzzword[j],"Resetting Timer and Score / Starting Game in 1...");
    delay(1000);
    Serial.println("Resetting Timer and Score / Starting Game");
    startTimer();
    updateScreen(score, i, remainingTime, i);
  }

  // If teammate gives correct answer, press the middle button to increment score
  // and display next question.
  if (correctButtonPressed) {
    correctButtonPressed = false;
    if (isTimerRunning) {
      score += 1; // increment score
      i += 1;     // advance to display next question
      if (i < 10) {
        updateScreen(score, i, remainingTime, i);
      } else if (i == 10) {  //All questions done
        writeScreen(buzzword[j], "Final Score: " + String(score) + "/" + String(i));
        isTimerRunning = false;
      }
      Serial.println("Correct: " + String(score));
    }
  }

  // If teammate passes or gives the wrong answer, press the right button to display
  // next question.
  if (wrongButtonPressed) {
    wrongButtonPressed = false;
    if (isTimerRunning) {
      i += 1; // advance to display next question
      if (i < 10) {
        updateScreen(score, i, remainingTime, i);
      } else if (i == 10) {  // All questions done
        writeScreen(buzzword[j], "Final Score: " + String(score) + "/" + String(i));
        isTimerRunning = false;
      }
      Serial.println("Wrong");
   }
  }

  // Update countdown timer
  if (isTimerRunning) {
    unsigned long currentTime = millis();
    // Calculate elapsed time in seconds
    unsigned long newElapsedSeconds = (currentTime - startTime) / 1000;

    // Check if a full second has passed and update if necessary
    if (newElapsedSeconds != elapsedSeconds) {
      elapsedSeconds = newElapsedSeconds;
      remainingTime = maxTime - elapsedSeconds;
      Serial.print("Time remaining: ");
      Serial.println(remainingTime);
      updateScreen(score, i, remainingTime, i);

      if (remainingTime <= 0) {
        remainingTime = 0;
        timerFinished();
      }
    }
  }

  delay(100);    
}

void startTimer() {
  startTime = millis(); // Record the start time
  isTimerRunning = true;
}

void timerFinished() {
  isTimerRunning = false;
  Serial.println("Time is Up!");
  // Todo: Trigger buzzer when time runs out
  writeScreen(buzzword[j], "Time is up!");
}

void setupWiFi() {

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Wi-Fi...");
    }

    Serial.println("Connected to Wi-Fi");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());
}

String getQuestions(String buzzword) {

  // Call ChatGPT to get a list of words and corresponding questions
  String chatGPTContent = chatGPTRequest(buzzword);
  String result;

  // Parse the response contents and store the 10 words and questions into 2D array
  for (int i=0; i<10; i++) {
    // Extract word.  The ChatGPT response puts the words between "**". Not sure if this is
    // always the case...
    result = extractStringBetweenMarkers(chatGPTContent, "**", "**");
    QA[i][0] = result;
    // Extract question.  The ChatGPT response preceeds the question with "Question: ".  Not
    // sure if this is always the case...
    result = extractStringBetweenMarkers(chatGPTContent, "Question: ", "?");
    QA[i][1] = result + "?";
    int index = chatGPTContent.indexOf("?");
    // Cuts off the parts of the chatGPTContent that has been extracted
    chatGPTContent = chatGPTContent.substring(index+1);  
    Serial.print(String(QA[i][0]) + " - ");
    Serial.println(String(QA[i][1]));
  }

  return chatGPTContent;
}

String chatGPTRequest(String buzzword) {
// Use ChatGPT to get a list of 10 words and questions based upon the buzzword.

  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect(OPENAI_HOST, OPENAI_PORT)) {
    Serial.println("Connection failed");
    return "Connection failed";
  }

  // Build ChatGPT prompt to generate 10 words or phrases containing the base word,
  // along with questions to guess those 10 words.
  String prompt = 
    "Give me 10 words or phrases that contain the word " + String(buzzword) + 
    " and questions to guess those 10 words.";

  StaticJsonDocument<1024> doc;
  doc["model"] = "gpt-4o-mini";

  JsonArray messages = doc.createNestedArray("messages");
  JsonObject msg = messages.createNestedObject();
  msg["role"] = "user";
  msg["content"] = prompt;

  String requestBody;
  serializeJson(doc, requestBody);
  // Serial.println(String(requestBody));  // Debug dump of ChatGPT request
  Serial.println("------------");

  // HTTP Post
  client.println("POST /v1/chat/completions HTTP/1.1");
  client.println("Host: api.openai.com");
  client.println("Authorization: Bearer " + String(OPENAI_API_KEY));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(String(requestBody).length());
  client.println();
  client.println(String(requestBody));

  String statusLine = client.readStringUntil('\n');
  if (statusLine.startsWith("HTTP/1.1 200")) {
    Serial.println("HTTP error");
    return "HTTP error";
  }

  while (client.connected() && !client.available()) delay(10);
  // Skip headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }
  // Read body
  String response;
  while (client.available()) response += client.readString();
  //Serial.println(response); // Debug print of response
  //Serial.println("------- Response end");

  // Go to start of response with '{'
  int index = response.indexOf('{');
  String json = response.substring(index);
  
  StaticJsonDocument<2048> responseDoc;
  DeserializationError err = deserializeJson(responseDoc, json);
  if (err) {
    Serial.println("Json error");
    Serial.println(err.c_str());
    return ("Json error");
  }

  // Extract "content"
  const char* reply = responseDoc["choices"][0]["message"]["content"];

  Serial.println(String(reply));  // Debug print content
  return String(reply);
} 

void handleStartButton() {
  unsigned long now = millis();
  if (now - lastPressedTimeS > DEBOUNCE_DELAY) {
    startButtonPressed = true;
  }
  lastPressedTimeS = now;
}

void handleCorrectButton() {
  //correctButtonPressed = true;
  unsigned long now = millis();
  if (now - lastPressedTimeC > DEBOUNCE_DELAY) {
    correctButtonPressed = true;
  }
  lastPressedTimeC = now;
}

void handleWrongButton() {
  unsigned long now = millis();
  if (now - lastPressedTimeW > DEBOUNCE_DELAY) {
    wrongButtonPressed = true;
  }
  lastPressedTimeW = now;
}

void updateScreen(int score, int i, unsigned long time, int j) {
  int displayTime = int(time);
  tft.fillScreen(TFT_BLACK);
  // Display score
  tft.setCursor(10, 10);
  tft.print("S: ");
  tft.print(score);
  tft.print(" / ");
  tft.print(i);

  // Display time
  tft.setCursor(160, 10);
  tft.print("T: ");
  tft.print(displayTime, 0);

  // Display word and question
  if (j >= 0) {
    tft.setCursor(10, 30);
    tft.print(QA[i][0]);
    tft.setCursor(10, 50);
    tft.print(QA[i][1]);
  }
}

void writeScreen(String buzzword, String text) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.print("Buzzword: ");
  tft.print(buzzword);
  tft.setCursor(10, 50);
  tft.print(text);
}

String extractStringBetweenMarkers(String data, String startMarker, String endMarker) {
  int startIndex = data.indexOf(startMarker); // Find the first occurrence of the start marker
  if (startIndex == -1) {
    return ""; // Return empty if the start marker is not found
  }

  startIndex += startMarker.length(); // Move the index to the character immediately after the start marker

  int endIndex = data.indexOf(endMarker, startIndex); // Find the end marker starting from the position after the start marker
  if (endIndex == -1) {
    return ""; // Return empty if the end marker is not found
  }

  // Extract the substring between the calculated indices
  return data.substring(startIndex, endIndex);
}
