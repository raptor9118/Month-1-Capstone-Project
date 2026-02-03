# Month-1-Capstone-Project
Electronic Buzzword game using ESP32

This is an electronic version of the game “Buzzword”.  Buzzword game is a fast-paced multi-team party game where during each round, one player on the team reads clues, while teammates guess answers that contain the buzzword.  Teams have 60 seconds to answer up to 10 questions.  The buzzword is given before the round starts.  The clue-giver reads clues, and teammates shout answers incorporating the buzzword (e.g., if the buzzword is "ball," answers might be "beach ball," "basketball").  

In my electronic version, there are 3 buttons:
- Start game button - gets a “buzzword” from a hardcoded array of words, and asks ChatGPT to generate 10 words containing the buzzword and questions to guess the word.  The 10 questions and answers are stored in an array.  It then displays the first question and answer, starts a 60 second countdown timer and the round begins.
- “Correct” button - increments the score and and question number and displays the next question
- “Wrong” button - skips to the next question, incrementing the question number but not the score.  

The person reading the clues or someone on the other team would interact with the buttons.
