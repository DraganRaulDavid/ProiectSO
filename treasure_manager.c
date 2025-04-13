#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

//Structure treasure data
typedef struct {
    int treasureId;
    char userName[50];
    float latitude;
    float longitude;
    char clueText[200];
    int value;
} Treasure;

//Creates hunt directory if it doesn't exist
int createHuntDirectory(char* huntId) 
{
    //Create directory if it doesn't exist
    struct stat st = {0};
    char dirPath[100];
    
    sprintf(dirPath, "./%s", huntId);
    
    if (stat(dirPath, &st) == -1) 
    {
        //Directory doesn't exist, create it
        if (mkdir(dirPath, 0700) == -1) 
        {
            perror("Failed to create hunt directory");
            return -1;
        }
        printf("Created new hunt: %s\n", huntId);
    }
    
    return 0;
}

//Create symbolic link to hunt's log file
void createSymLink(char* huntId) 
{
    char logPath[100];
    char linkPath[100];
    
    sprintf(logPath, "./%s/logged_hunt", huntId);
    sprintf(linkPath, "./logged_hunt-%s", huntId);
    
    //Remove existing symlink if it exists
    unlink(linkPath);
    
    //Create new symlink
    if (symlink(logPath, linkPath) == -1) 
    {
        perror("Failed to create symbolic link");
    }
}

//Log operation
void logOperation(char* huntId, char* operation) 
{
    char logPath[100];
    sprintf(logPath, "./%s/logged_hunt", huntId);
    
    int fd = open(logPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) 
    {
        perror("Failed to open log file");
        return;
    }
    
    //Create log entry
    char logEntry[300];
    sprintf(logEntry, "%s\n", operation);
    
    //Write to log file
    write(fd, logEntry, strlen(logEntry));
    close(fd);
    
    //Create symlink
    createSymLink(huntId);
}

//Add treasure to the specified hunt
void addTreasure(char* huntId) 
{
    if (createHuntDirectory(huntId) == -1)
    {
        return;
    }
    
    //Prepare treasure file path
    char filePath[100];
    sprintf(filePath, "./%s/treasures", huntId);
    
    //Open treasure file
    int fd = open(filePath, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Failed to open treasure file");
        return;
    }
    
    //Get number of treasures
    struct stat st;
    fstat(fd, &st);
    int treasureCount = st.st_size / sizeof(Treasure);
    
    //Create new treasure
    Treasure newTreasure;
    newTreasure.treasureId = treasureCount + 1;
    
    printf("Enter username: ");
    scanf("%s", newTreasure.userName);
    
    printf("Enter latitude: ");
    scanf("%f", &newTreasure.latitude);
    
    printf("Enter longitude: ");
    scanf("%f", &newTreasure.longitude);
    
    printf("Enter clue text: ");
    getchar();
    fgets(newTreasure.clueText, sizeof(newTreasure.clueText), stdin);
    //Remove newline
    newTreasure.clueText[strcspn(newTreasure.clueText, "\n")] = 0;
    
    printf("Enter value: ");
    scanf("%d", &newTreasure.value);
    
    //Move to end of file and write new treasure
    lseek(fd, 0, SEEK_END);
    write(fd, &newTreasure, sizeof(Treasure));
    close(fd);
    
    //Log operation
    char operation[100];
    sprintf(operation, "Added treasure %d to hunt %s", newTreasure.treasureId, huntId);
    logOperation(huntId, operation);
    
    printf("Treasure added successfully with ID: %d\n", newTreasure.treasureId);
}

//List all treasures in a hunt
void listTreasures(char* huntId) 
{
    char filePath[100];
    sprintf(filePath, "./%s/treasures", huntId);
    
    //Check if hunt exists
    struct stat st;
    if (stat(filePath, &st) == -1)
     {
        printf("Hunt not found: %s\n", huntId);
        return;
    }
    
    //Open treasure file
    int fd = open(filePath, O_RDONLY);
    if (fd == -1)
     {
        perror("Failed to open treasure file");
        return;
    }
    
    //Print hunt info
    printf("Hunt: %s\n", huntId);
    printf("File size: %lld bytes\n", st.st_size);
    
    //Read and print all treasures
    Treasure treasure;
    printf("Treasures in hunt %s:\n", huntId);
    printf("-------------------\n");
    
    int treasureCount = 0;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure))
     {
        printf("ID: %d\n", treasure.treasureId);
        printf("User: %s\n", treasure.userName);
        printf("Location: %.6f, %.6f\n", treasure.latitude, treasure.longitude);
        printf("Clue: %s\n", treasure.clueText);
        printf("Value: %d\n", treasure.value);
        printf("-------------------\n");
        treasureCount++;
    }
    
    if (treasureCount == 0)
     {
        printf("No treasures found in this hunt.\n");
    }
    
    close(fd);
    
    //Log operation
    char operation[100];
    sprintf(operation, "Listed treasures in hunt %s", huntId);
    logOperation(huntId, operation);
}

//View details of a specific treasure
void viewTreasure(char* huntId) 
{
    char filePath[100];
    sprintf(filePath, "./%s/treasures", huntId);
    
    //Check if hunt exists
    struct stat st;
    if (stat(filePath, &st) == -1) 
    {
        printf("Hunt not found: %s\n", huntId);
        return;
    }
    
    //Open treasure file
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) 
    {
        perror("Failed to open treasure file");
        return;
    }
    
    //Ask for treasure ID
    int treasureId;
    printf("Enter treasure ID to view: ");
    scanf("%d", &treasureId);
    
    //Find and display the treasure
    Treasure treasure;
    int found = 0;
    
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        if (treasure.treasureId == treasureId) {
            printf("\nTreasure Details:\n");
            printf("ID: %d\n", treasure.treasureId);
            printf("User: %s\n", treasure.userName);
            printf("Location: %.6f, %.6f\n", treasure.latitude, treasure.longitude);
            printf("Clue: %s\n", treasure.clueText);
            printf("Value: %d\n", treasure.value);
            found = 1;
            break;
        }
    }
    
    if (!found) 
    {
        printf("Treasure with ID %d not found in hunt %s\n", treasureId, huntId);
    }
    
    close(fd);
    
    //Log operation
    if (found) 
    {
        char operation[100];
        sprintf(operation, "Viewed treasure %d in hunt %s", treasureId, huntId);
        logOperation(huntId, operation);
    }
}

//Remove a treasure from a hunt
void removeTreasure(char* huntId, char* treasureIdStr)
{
    char filePath[100];
    sprintf(filePath, "./%s/treasures", huntId);
    
    //Check if hunt exists
    struct stat st;
    if (stat(filePath, &st) == -1) 
    {
        printf("Hunt not found: %s\n", huntId);
        return;
    }
    
    int treasureId = atoi(treasureIdStr);
    
    //Open treasure file
    int fd = open(filePath, O_RDWR);
    if (fd == -1)
     {
        perror("Failed to open treasure file");
        return;
    }
    
    //Create a temporary file
    char tempPath[100];
    sprintf(tempPath, "./%s/treasures.tmp", huntId);
    int tempFd = open(tempPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (tempFd == -1) 
    {
        perror("Failed to create temporary file");
        close(fd);
        return;
    }
    
    //Find and remove the treasure
    Treasure treasure;
    int found = 0;
    int newId = 1;
    
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure))
     {
        if (treasure.treasureId == treasureId) 
        {
            found = 1;
            continue;
        }
        
        treasure.treasureId = newId++;
        write(tempFd, &treasure, sizeof(Treasure));
    }
    
    close(fd);
    close(tempFd);
    
    if (!found) 
    {
        printf("Treasure with ID %d not found in hunt %s\n", treasureId, huntId);
        unlink(tempPath);  //Remove temp file
        return;
    }
    
    //Replace original file with temp file
    unlink(filePath);
    rename(tempPath, filePath);
    
    printf("Treasure with ID %d removed from hunt %s\n", treasureId, huntId);
    
    //Log operation
    char operation[100];
    sprintf(operation, "Removed treasure %d from hunt %s", treasureId, huntId);
    logOperation(huntId, operation);
}

//Remove an entire hunt
void removeHunt(char* huntId) 
{
    char dirPath[100];
    sprintf(dirPath, "./%s", huntId);
    
    //Check if hunt exists
    struct stat st;
    if (stat(dirPath, &st) == -1) 
    {
        printf("Hunt not found: %s\n", huntId);
        return;
    }
    
    //Remove treasure file
    char filePath[100];
    sprintf(filePath, "./%s/treasures", huntId);
    unlink(filePath);
    
    //Remove log file
    sprintf(filePath, "./%s/logged_hunt", huntId);
    unlink(filePath);
    
    //Remove directory
    if (rmdir(dirPath) == -1) 
    {
        perror("Failed to remove hunt directory");
        return;
    }
    
    //Remove symlink
    char linkPath[100];
    sprintf(linkPath, "./logged_hunt-%s", huntId);
    unlink(linkPath);
    
    printf("Hunt %s removed successfully\n", huntId);
}

int main(int argc, char *argv[]) 
{
    if (argc < 3) 
    {
        printf("Usage: %s --operation hunt_id [treasure_id]\n", argv[0]);
        return 1;
    }

    char* operation = argv[1];
    char* huntId = argv[2];
    
    if (strcmp(operation, "--add") == 0) 
    {
        addTreasure(huntId);
    } 
    else if (strcmp(operation, "--list") == 0) 
    {
        listTreasures(huntId);
    } 
    else if (strcmp(operation, "--view") == 0) 
    {
        viewTreasure(huntId);
    } 
    else if (strcmp(operation, "--remove_treasure") == 0) 
    {
        if (argc < 4) 
        {
            printf("Need treasure_id for remove_treasure operation\n");
            return 1;
        }
        removeTreasure(huntId, argv[3]);
    }
    else if (strcmp(operation, "--remove_hunt") == 0) 
    {
        removeHunt(huntId);
    }
    else 
    {
        printf("Unknown operation: %s\n", operation);
        return 1;
    }

    return 0;
}