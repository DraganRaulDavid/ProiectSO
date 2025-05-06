#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

// Global variables
pid_t monitor_pid = -1;  // Process ID of the monitor
int is_monitor_stopping = 0;  // Flag to check if monitor is stopping
char command_file[] = "command.txt";  // File for communication

typedef struct {
    int treasureId;
    char userName[50];
    float latitude;
    float longitude;
    char clueText[200];
    int value;
} Treasure;

void handle_child_termination(int signo) 
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    
    if (pid == monitor_pid) 
    {
        printf("Monitor process (PID: %d) has terminated.\n", pid);
        
        if (WIFEXITED(status)) 
        {
            printf("Monitor exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) 
        {
            printf("Monitor killed by signal: %d\n", WTERMSIG(status));
        }
        
        monitor_pid = -1; 
        is_monitor_stopping = 0;  
    }
}

// Set up signal handlers
void setup_signal_handlers() 
{
    struct sigaction sa;
    
    // Setup SIGCHLD handler
    sa.sa_handler = handle_child_termination;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
        perror("Failed to set up SIGCHLD handler");
        exit(EXIT_FAILURE);
    }
}

void print_directory_contents(const char* dir_name) 
{
    DIR *dir = opendir(dir_name);
    if (!dir) 
    {
        printf("Cannot open directory %s\n", dir_name);
        return;
    }
    
    struct dirent *entry;
    printf("Contents of %s:\n", dir_name);
    while ((entry = readdir(dir)) != NULL) 
    {
        printf("  %s\n", entry->d_name);
    }
    
    closedir(dir);
}

// Handler for command signals in monitor process
void handle_command_signal(int signo) 
{
    // Open and read the command file
    FILE *cmdFile = fopen(command_file, "r");
    if (cmdFile == NULL) 
    {
        perror("Monitor: Failed to open command file");
        return;
    }
    
    // Read command and parameter
    char cmd[50] = {0};
    char param[100] = {0};
    fscanf(cmdFile, "%s %s", cmd, param);
    fclose(cmdFile);
    
    // Process different commands
    if (strcmp(cmd, "list_hunts") == 0) 
    {
        printf("\n--- MONITOR: LISTING ALL HUNTS ---\n");
        
        // Open current directory
        DIR *dir = opendir(".");
        if (dir == NULL) 
        {
            perror("Monitor: Failed to open directory");
            return;
        }
        
        struct dirent *entry;
        int huntCount = 0;
        
        // Debug: Print all directory entries
        printf("Current directory contents:\n");
        while ((entry = readdir(dir)) != NULL)
        {
            printf("  %s", entry->d_name);
            
            // Skip . and .. directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            {
                printf(" (skipped)\n");
                continue;
            }
            
            struct stat st;
            if (stat(entry->d_name, &st) == 0) 
            {
                if (S_ISDIR(st.st_mode)) 
                {
                    printf(" (directory)");
                    
                    // Check if this directory has a treasures file
                    char treasureFile[150];
                    sprintf(treasureFile, "%s/treasures", entry->d_name);
                    
                    struct stat treasure_st;
                    if (stat(treasureFile, &treasure_st) == 0) 
                    {
                        // Count treasures based on file size and struct size
                        int treasureCount = treasure_st.st_size / sizeof(Treasure);
                        printf(" - Hunt with %d treasures\n", treasureCount);
                        
                        // Print actual hunt information
                        printf("Hunt: %s - Total treasures: %d\n", entry->d_name, treasureCount);
                        huntCount++;
                    } 
                    else 
                    {
                        printf(" - Not a hunt (no treasures file)\n");
                    }
                } 
                else 
                {
                    printf(" (file)\n");
                }
            } 
            else 
            {
                printf(" (cannot stat)\n");
            }
        }
        
        if (huntCount == 0) 
        {
            printf("No hunts found.\n");
        }
        
        closedir(dir);
        printf("--- END OF HUNT LISTING ---\n\n");
        
    } 
    else if (strcmp(cmd, "list_treasures") == 0) 
    {
        printf("\n--- MONITOR: LISTING TREASURES FOR HUNT: %s ---\n", param);
        
        // Check if hunt directory exists
        struct stat st;
        if (stat(param, &st) == -1 || !S_ISDIR(st.st_mode)) 
        {
            printf("Error: Hunt '%s' not found or not a directory\n", param);
            return;
        }
        
        // Check if treasures file exists
        char treasureFile[150];
        sprintf(treasureFile, "%s/treasures", param);
        
        if (stat(treasureFile, &st) == -1) 
        {
            printf("Error: No treasures file found for hunt '%s'\n", param);
            return;
        }
        
        // Open treasures file directly
        int fd = open(treasureFile, O_RDONLY);
        if (fd == -1) 
        {
            perror("Failed to open treasures file");
            return;
        }
        
        // Print hunt info
        printf("Hunt: %s\n", param);
        printf("File size: %lld bytes\n", st.st_size);
        
        // Read and print all treasures
        Treasure treasure;
        printf("Treasures in hunt %s:\n", param);
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
        
    } 
    else if (strcmp(cmd, "view_treasure") == 0) 
    {
        printf("\n--- MONITOR: VIEWING TREASURE IN HUNT: %s ---\n", param);
        
        // Check if hunt directory exists
        struct stat st;
        if (stat(param, &st) == -1 || !S_ISDIR(st.st_mode)) 
        {
            printf("Error: Hunt '%s' not found or not a directory\n", param);
            return;
        }
        
        // Check if treasures file exists
        char treasureFile[150];
        sprintf(treasureFile, "%s/treasures", param);
        
        if (stat(treasureFile, &st) == -1) 
        {
            printf("Error: No treasures file found for hunt '%s'\n", param);
            return;
        }
        
        // Open treasures file directly
        int fd = open(treasureFile, O_RDONLY);
        if (fd == -1) 
        {
            perror("Failed to open treasures file");
            return;
        }
        
        // Ask for treasure ID
        int treasureId;
        printf("Enter treasure ID to view: ");
        scanf("%d", &treasureId);
        
        // Find and display the treasure
        Treasure treasure;
        int found = 0;
        
        while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) 
        {
            if (treasure.treasureId == treasureId) 
            {
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
            printf("Treasure with ID %d not found in hunt %s\n", treasureId, param);
        }
        
        close(fd);
        
    } else if (strcmp(cmd, "stop_monitor") == 0) 
    {
        printf("\n--- MONITOR: STOPPING ---\n");
        printf("Monitor process (PID: %d) is shutting down...\n", getpid());
        
        // Simulate a delay before shutting down
        usleep(500000);  // 0.5 second delay
        
        printf("Monitor process terminated.\n");
        exit(EXIT_SUCCESS);
    }
}

// Start the monitor process
void start_monitor() {
    if (monitor_pid != -1) 
    {
        printf("Error: Monitor is already running (PID: %d)\n", monitor_pid);
        return;
    }
    
    monitor_pid = fork();
    
    if (monitor_pid < 0) 
    {
        // Error creating process
        perror("Failed to fork monitor process");
        exit(EXIT_FAILURE);
    } 
    else if (monitor_pid == 0) 
    {
        
        // Set up signal handler for commands
        struct sigaction sa;
        sa.sa_handler = handle_command_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        
        if (sigaction(SIGUSR1, &sa, NULL) == -1)
        {
            perror("Monitor: Failed to set up SIGUSR1 handler");
            exit(EXIT_FAILURE);
        }
        
        printf("Monitor process started with PID: %d\n", getpid());
        printf("Ready to receive commands.\n");
        
        // Keep the monitor running until it receives a stop command
        while (1) 
        {
            sleep(1);
        }
        
        exit(EXIT_SUCCESS);
    }
    else 
    {
        printf("Started monitor process with PID: %d\n", monitor_pid);
    }
}

// Send a command to the monitor process
void send_command(const char* command, const char* param) 
{
    if (monitor_pid == -1) 
    {
        printf("Error: Monitor is not running. Use 'start_monitor' first.\n");
        return;
    }
    
    if (is_monitor_stopping) 
    {
        printf("Error: Monitor is in the process of stopping. Please wait.\n");
        return;
    }
    
    // Write command to file for the monitor to read
    FILE *cmdFile = fopen(command_file, "w");
    if (cmdFile == NULL) 
    {
        perror("Failed to open command file");
        return;
    }
    
    if (param != NULL) 
    {
        fprintf(cmdFile, "%s %s", command, param);
    } 
    else 
    {
        fprintf(cmdFile, "%s", command);
    }
    
    fclose(cmdFile);
    
    // Send signal to monitor to process the command
    if (kill(monitor_pid, SIGUSR1) == -1) 
    {
        perror("Failed to send signal to monitor");
    }
}

// List all hunts
void list_hunts() 
{
    send_command("list_hunts", NULL);
}

// List all treasures in a hunt
void list_treasures() 
{
    char huntId[50];
    printf("Enter hunt ID: ");
    scanf("%s", huntId);
    
    send_command("list_treasures", huntId);
}

// View a specific treasure
void view_treasure() 
{
    char huntId[50];
    printf("Enter hunt ID: ");
    scanf("%s", huntId);
    
    send_command("view_treasure", huntId);
}

// Stop the monitor process
void stop_monitor() 
{
    if (monitor_pid == -1) 
    {
        printf("Error: Monitor is not running.\n");
        return;
    }
    
    if (is_monitor_stopping) 
    {
        printf("Error: Monitor is already in the process of stopping.\n");
        return;
    }
    
    is_monitor_stopping = 1;
    send_command("stop_monitor", NULL);
}

int main() 
{
    // Set up signal handlers
    setup_signal_handlers();
    
    char input[50];
    
    printf("Treasure Hub - Interactive Interface\n");
    printf("Available commands: start_monitor, list_hunts, list_treasures, view_treasure, stop_monitor, exit\n");
    
    while (1) 
    {
        printf("\n> ");
        scanf("%s", input);
        
        if (strcmp(input, "start_monitor") == 0) 
        {
            start_monitor();
        } 
        else if (strcmp(input, "list_hunts") == 0) 
        {
            list_hunts();
        } 
        else if (strcmp(input, "list_treasures") == 0) 
        {
            list_treasures();
        } 
        else if (strcmp(input, "view_treasure") == 0) 
        {
            view_treasure();
        } 
        else if (strcmp(input, "stop_monitor") == 0) 
        {
            stop_monitor();
        } 
        else if (strcmp(input, "exit") == 0)
        {
            if (monitor_pid != -1) 
            {
                printf("Error: Cannot exit while monitor is running. Use 'stop_monitor' first.\n");
            } 
            else 
            {
                printf("Exiting Treasure Hub...\n");
                break;
            }
        } 
        else 
        {
            printf("Unknown command: %s\n", input);
        }
    }
    
    return 0;
}