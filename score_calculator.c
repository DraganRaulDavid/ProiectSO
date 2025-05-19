#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

typedef struct {
    int treasureId;
    char userName[50];
    float latitude;
    float longitude;
    char clueText[200];
    int value;
} Treasure;

// Function to calculate and print scores for a hunt
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }
    
    char *huntId = argv[1];
    
    printf("Score calculation for hunt: %s\n", huntId);
    printf("-----------------------------------\n");
    
    // Open treasures file
    char treasureFile[150];
    sprintf(treasureFile, "%s/treasures", huntId);
    
    // Check if file exists
    struct stat st;
    if (stat(treasureFile, &st) == -1) {
        printf("Error: No treasures file found for hunt '%s'\n", huntId);
        return 1;
    }
    
    // Open file for reading
    int fd = open(treasureFile, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open treasures file");
        return 1;
    }
    
    // Read all treasures and calculate scores by user
    Treasure treasure;
    
    // Create an array to store user scores
    #define MAX_USERS 50
    char userNames[MAX_USERS][50];
    int userScores[MAX_USERS];
    int userCount = 0;
    
    // Read all treasures and update scores
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        // Check if user already exists in our array
        int userIndex = -1;
        for (int i = 0; i < userCount; i++) {
            if (strcmp(userNames[i], treasure.userName) == 0) {
                userIndex = i;
                break;
            }
        }
        
        if (userIndex == -1) {
            // New user
            if (userCount < MAX_USERS) {
                strcpy(userNames[userCount], treasure.userName);
                userScores[userCount] = treasure.value;
                userCount++;
            }
        } else {
            // Existing user, update score
            userScores[userIndex] += treasure.value;
        }
    }
    
    close(fd);
    
    // Print results
    if (userCount == 0) {
        printf("No treasures found in this hunt.\n");
        return 0;
    }
    
    printf("User Scores:\n");
    printf("------------\n");
    
    for (int i = 0; i < userCount; i++) {
        printf("User: %-15s Score: %d\n", userNames[i], userScores[i]);
    }
    
    // Find the winner
    int maxScore = -1;
    int winnerIndex = -1;
    
    for (int i = 0; i < userCount; i++) {
        if (userScores[i] > maxScore) {
            maxScore = userScores[i];
            winnerIndex = i;
        }
    }
    
    printf("\nWinner: %s with score %d\n", userNames[winnerIndex], maxScore);
    printf("-----------------------------------\n");
    
    return 0;
}