# Jumanji-Arduino

### 	Jumanji is a game in which there are MCQs that the player should answer while the victim is attached to a wire connected to a servo motor arm, and this arm moves with speed depending on the answers correctness rate, so the speed gets slower if the player is answering correct to allow more time for next questions before falling, and gets faster when the player is answering wrongly to make the player think carefully as the victim is more close to falling. The question is displayed on an LCD screen, the score is displayed on 7-Segment, the input of answers are read from a keypad, a green led is turned on when the player chooses correctly, a red led is turned on when the player chooses wrongly, a buzzer makes a celebration tone when the player wins and makes a noisy tone when the player loses the game and the victim falls.


### The game ends in multiple cases:
#### 1- If the motor rotated 90°, the player will lose regardless of how many correct answers he scored as the victim falls.
#### 2- If the player answered all questions and the score ≥ 7, the player wins.
#### 3- If the player answered all questions but the score < 7, the victim falls.  
