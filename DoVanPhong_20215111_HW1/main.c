#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

typedef struct Account
{
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int status;
    int failedAttempts;
    struct Account *next;
} Account;

Account *head = NULL;
Account *loggedInUser = NULL;

Account *makeAccount(char *username, char *password, int status, int failedAttempts)
{
    Account *account = (Account *)malloc(sizeof(Account));
    strcpy(account->username, username);
    strcpy(account->password, password);
    account->status = status;
    account->failedAttempts = failedAttempts;
    account->next = NULL;
    return account;
}

Account *insertLast(Account *r, char *username, char *password, int status, int failedAttempts)
{
    if (r == NULL)
        return makeAccount(username, password, status, failedAttempts);
    r->next = insertLast(r->next, username, password, status, failedAttempts);
    return r;
}

int loadAccounts()
{
    FILE *file = fopen("account.txt", "r");
    if (file == NULL)
    {
        perror("Not found file");
        return 0;
    }
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int status;

    while (fscanf(file, "%s %s %d", username, password, &status) != EOF)
    {
        head = insertLast(head, username, password, status, 0);
    }

    fclose(file);
    return 1;
}

void saveAccounts()
{
    FILE *file = fopen("account.txt", "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    Account *current = head;
    while (current != NULL)
    {
        fprintf(file, "%s %s %d\n", current->username, current->password, current->status);
        current = current->next;
    }

    fclose(file);
}

Account *findAccount(const char *username)
{
    Account *current = head;
    while (current != NULL)
    {
        if (strcmp(current->username, username) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void registerAccount()
{
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    printf("Enter username: ");
    scanf("%s", username);
    if (findAccount(username) != NULL)
    {
        printf("Account existed\n");
        return;
    }
    printf("Enter password: ");
    scanf("%s", password);

    head = insertLast(head, username, password, 1, 0);
    saveAccounts();
    printf("Successful registration\n");
}

void signIn() {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    printf("Enter username: ");
    scanf("%s", username);

    Account* account = findAccount(username);
    if (account == NULL) {
        printf("Cannot find account\n");
        return;
    }

    if (account->status == 0) {
        printf("Account is blocked\n");
        return;
    }

    while (account->failedAttempts < 3) {
        printf("Enter password (press Esc to cancel): ");

        int i = 0;
        char ch;
        while ((ch = getchar()) != '\r') {
            if (ch == 27) {
                printf("\nCancelled. Returning to menu.\n");
                return;
            }
            if (ch == '\b') {
                if (i > 0) {
                    i--;
                    printf("\b \b");
                }
            } else {
                password[i++] = ch;
                printf("*");
            }
        }
        password[i] = '\0';
        printf("\n");

        if (strcmp(account->password, password) == 0) {
            account->failedAttempts = 0;
            loggedInUser = account;
            printf("Hello %s\n", username);
            saveAccounts();
            return;
        } else {
            printf("Password is incorrect\n");
            account->failedAttempts++;
            saveAccounts();
        }
    }

    account->status = 0;
    saveAccounts();
    printf("Password is incorrect. Account is blocked\n");
}

void searchInfoAccount()
{
    if (loggedInUser == NULL)
    {
        printf("Error: You must be signed in to search for an account.\n");
        return;
    }

    char username[MAX_USERNAME_LEN];
    printf("Enter username to search: ");
    scanf("%s", username);

    Account *account = findAccount(username);
    if (account == NULL)
    {
        printf("Cannot find account\n");
        return;
    }

    printf("Account is %s\n", account->status == 1 ? "active" : "blocked");
}

void signOut()
{
    if (loggedInUser == NULL)
    {
        printf("Account is not sign in\n");
        return;
    }

    char username[MAX_USERNAME_LEN];
    printf("Enter your signed username to sign out : ");
    scanf("%s", username);

    Account *account = findAccount(username);
    if (account == NULL)
    {
        printf("Cannot find account\n");
        return;
    }

    if (strcmp(loggedInUser->username, username) != 0)
    {
        printf("Incorrect signed username, Try again!!\n");
        return;
    }

    loggedInUser = NULL;
    printf("Goodbye %s\n", username);
}

void displayMenu()
{
    printf("USER MANAGEMENT PROGRAM\n");
    printf("---------------------------------------------\n");
    printf("1. Register\n");
    printf("2. Sign in\n");
    printf("3. Search\n");
    printf("4. Sign out\n");
    printf("Your choice (1-4, other to quit): ");
}

void freeAccounts()
{
    Account *current = head;
    while (current != NULL)
    {
        Account *temp = current;
        current = current->next;
        free(temp);
    }
    head = NULL;
}

int main()
{
    int exitsFile = loadAccounts();

    int option;
    if (exitsFile)
    {
        do
        {
            displayMenu();
            fflush(stdin);
            scanf("%d", &option);

            switch (option)
            {
            case 1:
                registerAccount();
                break;
            case 2:
                signIn();
                break;
            case 3:
                searchInfoAccount();
                break;
            case 4:
                signOut();
                break;
            default:
                printf("Exiting program.\n");
                freeAccounts();
                break;
            }
        } while (option >= 1 && option <= 4);
    }

    return 0;
}
