#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INGREDIENTS 100 // For inventory items (ingredients, potions, trophies)
#define MAX_NAME_LEN 64     // For names of items, monsters, etc.
#define MAX_INPUT_LEN 1024  // Maximum length for user input lines
#define MAX_COMPONENTS 10   // Maximum number of components in a potion formula
#define MAX_FORMULAS 50     // Maximum number of potion formulas
#define MAX_BESTIARY 100    // Maximum number of bestiary entries

//Data Structures//

typedef struct {
    char name[MAX_NAME_LEN];
    int quantity;
} Item;

typedef struct {
    char potionName[MAX_NAME_LEN];
    Item components[MAX_COMPONENTS];
    int componentCount;
} Formula;

typedef struct {
    char monsterName[MAX_NAME_LEN];
    char effectivePotion[MAX_NAME_LEN]; // Effective potion
    char effectiveSign[MAX_NAME_LEN];   // Effective sign
} BestiaryEntry;

//Global Variables//

Item inventory[MAX_INGREDIENTS];
int inventory_count = 0;

Formula formulaBook[MAX_FORMULAS];
int formula_count = 0;

BestiaryEntry bestiary[MAX_BESTIARY];
int bestiaryCount = 0;

//Utility Functions//

//Trims leading and trailing whitespace
void trim(char* str) {
    char* start = str;
    while (isspace(*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    //Remove trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end >= str && isspace(*end)) {
        *end = '\0';
        end--;
    }
}


int endsWithQuestionMark(char *str) {  // Check if the string (after trimming trailing whitespace) ends with a '?' character.
    int len = strlen(str);
    // Move backwards past any trailing whitespace.
    while (len > 0 && isspace(str[len - 1])) {
        len--;
    }
    // Check if there's at least one character and if it is '?'
    return (len > 0 && str[len - 1] == '?');
}


// Case-insensitive substring search
char *strcasestr_custom(const char *haystack, const char *needle) {
    if (!*needle)
        return (char *)haystack;
    for (; *haystack; haystack++) {
        if (tolower(*haystack) == tolower(*needle)) {
            const char *h = haystack, *n = needle;
            while (*h && *n && tolower(*h) == tolower(*n)) {
                h++; n++;
            }
            if (!*n)
                return (char *)haystack;
        }
    }
    return NULL;
}

// Compare function for qsort (case-insensitive by item name)
int compareItems(const void *a, const void *b) {
    const Item itemA = *(const Item *)a;
    const Item itemB = *(const Item *)b;
    return strcasecmp(itemA.name, itemB.name);
}

// Compare function for trophies
int compareTrophies(const void *a, const void *b) { // Compare two trophy items by stripping the " trophy" suffix from their names and comparing them case-insensitively.
    const Item itemA = *(const Item *)a;
    const Item itemB = *(const Item *)b;
    char monsterA[MAX_NAME_LEN], monsterB[MAX_NAME_LEN];
    strncpy(monsterA, itemA.name, MAX_NAME_LEN);
    strncpy(monsterB, itemB.name, MAX_NAME_LEN);
    monsterA[MAX_NAME_LEN - 1] = '\0';
    monsterB[MAX_NAME_LEN - 1] = '\0';
    char *pos = strcasestr_custom(monsterA, " trophy");
    if (pos) *pos = '\0';
    pos = strcasestr_custom(monsterB, " trophy");
    if (pos) *pos = '\0';
    return strcasecmp(monsterA, monsterB);
}

// Compare function for formula components
int compareComponents(const void *a, const void *b) {
    const Item compA = *(const Item *)a;
    const Item compB = *(const Item *)b;
    if (compA.quantity != compB.quantity) {
        return compB.quantity - compA.quantity; //descending
    }
    return strcasecmp(compA.name, compB.name);
}

//Inventory Functions//

//Adds or updates an item in the inventory.
void addItem(char* name, int quantity) {
    for (int i = 0; i < inventory_count; i++) {
        if (strcasecmp(inventory[i].name, name) == 0) {
            inventory[i].quantity += quantity;
            return;
        }
    }
    if (inventory_count < MAX_INGREDIENTS) {
        strncpy(inventory[inventory_count].name, name, MAX_NAME_LEN);
        inventory[inventory_count].name[MAX_NAME_LEN - 1] = '\0';
        inventory[inventory_count].quantity = quantity;
        inventory_count++;
    }
}

//Removes a given quantity of an item from the inventory.
int removeItem(char* name, int quantity) {
    for (int i = 0; i < inventory_count; i++) {
        if (strcasecmp(inventory[i].name, name) == 0) {
            if (inventory[i].quantity >= quantity) {
                inventory[i].quantity -= quantity;
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

// Checks if the inventory has at least the required quantity.
int hasEnoughItem(char* name, int quantity) {
    for (int i = 0; i < inventory_count; i++) {
        if (strcasecmp(inventory[i].name, name) == 0) {
            return (inventory[i].quantity >= quantity);
        }
    }
    return 0;
}

//Classification Helpers//

//An item is a potion if its name does not end with " trophy" and its name matches one of the known potion formulas.
int isPotion(Item *item) {
    int len = strlen(item->name);
    if (len >= 7 && strcasecmp(item->name + len - 7, " trophy") == 0)
         return 0;
    for (int i = 0; i < formula_count; i++) {
         if (strcasecmp(item->name, formulaBook[i].potionName) == 0)
             return 1;
    }
    return 0;
}

//An item is a trophy if its name ends with " trophy".
int isTrophy(Item *item) {
    int len = strlen(item->name);
    return (len >= 7 && strcasecmp(item->name + len - 7, " trophy") == 0);
}

//Query Functions//

//Processes queries ending with '?'.
int processQuery(char* input) { 
    // processQuery: Determines the query type based on the input string (e.g., monster effectiveness, ingredient totals, potion formulas)
    // and routes the query to the appropriate handling block using complex string parsing.
    char* query = input;
    trim(query);

    //Potion/Sign Effectiveness Query: "What is effective against <monster> ?"
    if (strncasecmp(query, "What is effective against", strlen("What is effective against")) == 0) { // Handle "What is effective against" query: extract the monster name and look up its effective counter(s) in the bestiary.
        int offset = strlen("What is effective against ");
        char monster[MAX_NAME_LEN];
        strncpy(monster, query + offset, MAX_NAME_LEN);
        monster[MAX_NAME_LEN - 1] = '\0';
        char* qMark = strchr(monster, '?');
        if (qMark)
            *qMark = '\0';
        trim(monster);
        int index = -1;
        for (int i = 0; i < bestiaryCount; i++) {
            if (strcasecmp(bestiary[i].monsterName, monster) == 0) {
                index = i;
                break;
            }
        }
        if (index == -1) {
            printf("No knowledge of %s\n", monster);
            return 1;
        }
        char counters[2][MAX_NAME_LEN];
        int count = 0;
        if (strlen(bestiary[index].effectivePotion) > 0) {
            strncpy(counters[count], bestiary[index].effectivePotion, MAX_NAME_LEN);
            counters[count][MAX_NAME_LEN - 1] = '\0';
            count++;
        }
        if (strlen(bestiary[index].effectiveSign) > 0) {
            strncpy(counters[count], bestiary[index].effectiveSign, MAX_NAME_LEN);
            counters[count][MAX_NAME_LEN - 1] = '\0';
            count++;
        }
        if (count == 0) {
            printf("No knowledge of %s\n", monster);
            return 1;
        }
        if (count == 2 && strcasecmp(counters[0], counters[1]) > 0) {
            char temp[MAX_NAME_LEN];
            strncpy(temp, counters[0], MAX_NAME_LEN);
            strncpy(counters[0], counters[1], MAX_NAME_LEN);
            strncpy(counters[1], temp, MAX_NAME_LEN);
        }
        printf("%s", counters[0]);
        for (int i = 1; i < count; i++) {
            printf(", %s", counters[i]);
        }
        printf("\n");
        return 1;
    }
    //Specific Ingredient Query: "Total ingredient <ingredient> ?"
    if (strncasecmp(query, "Total ingredient", 16) == 0) {
        int offset = strlen("Total ingredient ");
        char remainder[MAX_INPUT_LEN];
        strncpy(remainder, query + offset, MAX_INPUT_LEN);
        remainder[MAX_INPUT_LEN - 1] = '\0';
        char* qMark = strchr(remainder, '?');
        if (qMark) *qMark = '\0';
        trim(remainder);
        if (strlen(remainder) > 0) {
            int total = 0;
            for (int i = 0; i < inventory_count; i++) {
                if (strcasecmp(inventory[i].name, remainder) == 0) {
                    total = inventory[i].quantity;
                    break;
                }
            }
            printf("%d\n", total);
        } else {
            //List all ingredients sorted by name (not potions and not trophies)
            int count = 0;
            for (int i = 0; i < inventory_count; i++) {
                if (inventory[i].quantity == 0)
                    continue;
                int len = strlen(inventory[i].name);
                if (len >= 7 && strcasecmp(inventory[i].name + len - 7, " trophy") == 0)
                    continue;
                int isPot = 0;
                for (int j = 0; j < formula_count; j++) {
                    if (strcasecmp(inventory[i].name, formulaBook[j].potionName) == 0) {
                        isPot = 1;
                        break;
                    }
                }
                if (!isPot)
                    count++;
            }
            if (count == 0) {
                printf("None\n");
            } else {
                Item *ingArr[count];
                int idx = 0;
                for (int i = 0; i < inventory_count; i++) {
                    int len = strlen(inventory[i].name);
                    if (len >= 7 && strcasecmp(inventory[i].name + len - 7, " trophy") == 0)
                        continue;
                    int isPot = 0;
                    for (int j = 0; j < formula_count; j++) {
                        if (strcasecmp(inventory[i].name, formulaBook[j].potionName) == 0) {
                            isPot = 1;
                            break;
                        }
                    }
                    if (!isPot) {
                        ingArr[idx++] = &inventory[i];
                    }
                }
                qsort(ingArr, count, sizeof(Item *), compareItems);
                for (int i = 0; i < count; i++) {
                    printf("%d %s", ingArr[i]->quantity, ingArr[i]->name);
                    if (i < count - 1)
                        printf(", ");
                }
                printf("\n");
            }
        }
        return 1;
    }
    //Specific Potion Query: "Total potion <potion> ?"
    else if (strncasecmp(query, "Total potion", 12) == 0) {
        int offset = strlen("Total potion ");
        char remainder[MAX_INPUT_LEN];
        strncpy(remainder, query + offset, MAX_INPUT_LEN);
        remainder[MAX_INPUT_LEN - 1] = '\0';
        char* qMark = strchr(remainder, '?');
        if (qMark) *qMark = '\0';
        trim(remainder);
        if (strlen(remainder) > 0) {
            int total = 0;
            for (int i = 0; i < inventory_count; i++) {
                if (strcasecmp(inventory[i].name, remainder) == 0) {
                    total = inventory[i].quantity;
                    break;
                }
            }
            printf("%d\n", total);
        } else {
            //List all potions sorted by name.
            int count = 0;
            for (int i = 0; i < inventory_count; i++) {
                if (isPotion(&inventory[i])) {
                    count++;
                }
            }
            if (count == 0) {
                printf("None\n");
            } else {
                Item *potArr[count];
                int idx = 0;
                for (int i = 0; i < inventory_count; i++) {
                    if (isPotion(&inventory[i])) {
                        potArr[idx++] = &inventory[i];
                    }
                }
                qsort(potArr, count, sizeof(Item *), compareItems);
                for (int i = 0; i < count; i++) {
                    printf("%d %s", potArr[i]->quantity, potArr[i]->name);
                    if (i < count - 1)
                        printf(", ");
                }
                printf("\n");
            }
        }
        return 1;
    }
    //Specific Trophy Query: "Total trophy <monster> ?"
    else if (strncasecmp(query, "Total trophy", 12) == 0) {
        int offset = strlen("Total trophy ");
        char remainder[MAX_INPUT_LEN];
        strncpy(remainder, query + offset, MAX_INPUT_LEN);
        remainder[MAX_INPUT_LEN - 1] = '\0';
        char* qMark = strchr(remainder, '?');
        if (qMark) *qMark = '\0';
        trim(remainder);
        if (strlen(remainder) > 0) {
            char trophyName[MAX_NAME_LEN];
            snprintf(trophyName, MAX_NAME_LEN, "%.42s trophy", remainder);
            int total = 0;
            for (int i = 0; i < inventory_count; i++) {
                if (strcasecmp(inventory[i].name, trophyName) == 0) {
                    total = inventory[i].quantity;
                    break;
                }
            }
            printf("%d\n", total);
        } else {
            //List all trophies sorted by monster names.
            int count = 0;
            for (int i = 0; i < inventory_count; i++) {
                if (isTrophy(&inventory[i])) {
                    count++;
                }
            }
            if (count == 0) {
                printf("None\n");
            } else {
                Item *trophyArr[count];
                int idx = 0;
                for (int i = 0; i < inventory_count; i++) {
                    if (isTrophy(&inventory[i])) {
                        trophyArr[idx++] = &inventory[i];
                    }
                }
                qsort(trophyArr, count, sizeof(Item *), compareTrophies);
                for (int i = 0; i < count; i++) {
                    char monsterName[MAX_NAME_LEN];
                    strncpy(monsterName, trophyArr[i]->name, MAX_NAME_LEN);
                    monsterName[MAX_NAME_LEN - 1] = '\0';
                    char *suffix = strcasestr_custom(monsterName, " trophy");
                    if (suffix)
                        *suffix = '\0';
                    printf("%d %s", trophyArr[i]->quantity, monsterName);
                    if (i < count - 1)
                        printf(", ");
                }
                printf("\n");
            }
        }
        return 1;
    }
    //Potion Formula Query: "What is in <potion> ?"
    else if (strncasecmp(query, "What is in", 10) == 0) {
        int offset = 10;
        char potionQuery[MAX_NAME_LEN];
        strncpy(potionQuery, query + offset, MAX_NAME_LEN);
        potionQuery[MAX_NAME_LEN - 1] = '\0';
        char* qMark = strchr(potionQuery, '?');
        if (qMark) *qMark = '\0';
        trim(potionQuery);
        Formula *f = NULL;
        for (int i = 0; i < formula_count; i++) {
            if (strcasecmp(formulaBook[i].potionName, potionQuery) == 0) {
                f = &formulaBook[i];
                break;
            }
        }
        if (!f) {
            printf("No formula for %s\n", potionQuery);
            return 1;
        }
        int compCount = f->componentCount;
        if (compCount == 0) {
            printf("No formula for %s\n", potionQuery);
            return 1;
        }
        Item *compArr[compCount];
        for (int i = 0; i < compCount; i++) {
            compArr[i] = &f->components[i];
        }
        qsort(compArr, compCount, sizeof(Item *), compareComponents);
        for (int i = 0; i < compCount; i++) {
            printf("%d %s", compArr[i]->quantity, compArr[i]->name);
            if (i < compCount - 1)
                printf(", ");
        }
        printf("\n");
        return 1;
    }
    else {
        printf("INVALID\n");
    }
    return 1;
}

//Action Handlers//

//Loot Action: "Geralt loots" followed by an ingredient_list.
int processLoot(char* input) {
    char* itemList = strstr(input, "Geralt loots ");
    if (!itemList) return 0; //do we need??
    itemList += strlen("Geralt loots ");
    trim(itemList);
        
    char* token = strtok(itemList, ",");
    while (token != NULL) {
        trim(token);
        int quantity;
        char name[MAX_NAME_LEN];
        if (sscanf(token, "%d %s", &quantity, name) != 2 || quantity <= 0)
            return 0;
        addItem(name, quantity);
        token = strtok(NULL, ",");
    }
    printf("Alchemy ingredients obtained\n");
    return 1;
}

//Trade Action: "Geralt trades" followed by a trophy_list, "for", then an ingredient_list.
int processTrade(char* input) {
    // processTrade: Processes trade commands by splitting the input into trophy and ingredient parts,
    // validating available trophy quantities, and updating the inventory accordingly.

    if (strncmp(input, "Geralt trades ", 14) != 0) return 0;
    char* tradeLine = input + 14;
    trim(tradeLine);
    
    char* forKeyword = strstr(tradeLine, "for"); // Split the input into trophy and ingredient segments based on the "for" keyword.
    if (!forKeyword) return 0;
    *forKeyword = '\0';
    char* trophyPart = tradeLine;
    char* ingredientPart = forKeyword + 3; //+3 since for consist of 3 characters
    trim(trophyPart);
    trim(ingredientPart);
    Item tempTrophies[MAX_INGREDIENTS];
    int trophyCount = 0;
    char* ttoken = strtok(trophyPart, ",");
    while (ttoken != NULL) {
        trim(ttoken);
        int qty;
        char name[MAX_NAME_LEN];
        if (sscanf(ttoken, "%d %[^\n]", &qty, name) != 2 || qty <= 0)
            return 0;
        
        if (!hasEnoughItem(name, qty)) {
            printf("Not enough trophies\n");
            return 1;
        }
        strncpy(tempTrophies[trophyCount].name, name, MAX_NAME_LEN);
        tempTrophies[trophyCount].name[MAX_NAME_LEN - 1] = '\0';
        tempTrophies[trophyCount].quantity = qty;
        trophyCount++;
        ttoken = strtok(NULL, ",");
    }
    char* itoken = strtok(ingredientPart, ",");
    while (itoken != NULL) {
        trim(itoken);
        int qty;
        char name[MAX_NAME_LEN];
        if (sscanf(itoken, "%d %s", &qty, name) != 2 || qty <= 0)
            return 0;
        addItem(name, qty);
        itoken = strtok(NULL, ",");
    }
    for (int i = 0; i < trophyCount; i++) {
        removeItem(tempTrophies[i].name, tempTrophies[i].quantity);
    }
    printf("Trade successful\n");
    return 1;
}

//Brew Action: "Geralt brews" followed by a potion.
int processBrew(char* input) {
    if (strncmp(input, "Geralt brews ", 13) != 0) return 0;
    char* potion = input + 13;
    trim(potion);
    Formula* f = NULL;
    for (int i = 0; i < formula_count; i++) {
        if (strcasecmp(formulaBook[i].potionName, potion) == 0) {
            f = &formulaBook[i];
            break;
        }
    }
    if (!f) {
        printf("No formula for %s\n", potion);
        return 1;
    }
    for (int i = 0; i < f->componentCount; i++) {
        if (!hasEnoughItem(f->components[i].name, f->components[i].quantity)) {
            printf("Not enough ingredients\n");
            return 1;
        }
    }
    for (int i = 0; i < f->componentCount; i++) {
        removeItem(f->components[i].name, f->components[i].quantity);
    }
    addItem(potion, 1);
    printf("Alchemy item created: %s\n", potion);
    return 1;
}

//Learn Action: Handles both effectiveness and potion formula learning.
int processLearn(char* input) {
// processLearn: Handles learning commands for both combat effectiveness and potion formulas.
// Determines if the input is for updating bestiary effectiveness (with "is effective against")
// or for adding a new potion formula (with "consists of") and processes each accordingly.

    if (strncmp(input, "Geralt learns ", 14) != 0) return 0;
    char* learnPart = input + 14;
    trim(learnPart);
    char* effectivePtr = strstr(learnPart, "is effective against"); // Processing effective counter: extract the counter (potion or sign) and enemy names, then update or add the corresponding bestiary entry.
    if (effectivePtr != NULL) {
        *effectivePtr = '\0';
        effectivePtr += strlen("is effective against");
        trim(effectivePtr);
        char enemy[MAX_NAME_LEN];
        strncpy(enemy, effectivePtr, MAX_NAME_LEN);
        enemy[MAX_NAME_LEN - 1] = '\0';
        char counter[MAX_NAME_LEN], type[MAX_NAME_LEN];
        if (sscanf(learnPart, "%s %s", counter, type) != 2) {
            printf("INVALID\n");
            return 1;
        }
        int isSign = 0, isPotion = 0;
        if (strcasecmp(type, "sign") == 0)
            isSign = 1;
        else if (strcasecmp(type, "potion") == 0)
            isPotion = 1;
        else {
            printf("INVALID\n");
            return 1;
        }
        int index = -1;
        for (int i = 0; i < bestiaryCount; i++) {
            if (strcasecmp(bestiary[i].monsterName, enemy) == 0) {
                index = i;
                break;
            }
        }
        if (index == -1) {
            if (bestiaryCount < MAX_BESTIARY) {
                strncpy(bestiary[bestiaryCount].monsterName, enemy, MAX_NAME_LEN);
                bestiary[bestiaryCount].monsterName[MAX_NAME_LEN - 1] = '\0';
                if (isSign) {
                    strncpy(bestiary[bestiaryCount].effectiveSign, counter, MAX_NAME_LEN);
                    bestiary[bestiaryCount].effectiveSign[MAX_NAME_LEN - 1] = '\0';
                } else {
                    strncpy(bestiary[bestiaryCount].effectivePotion, counter, MAX_NAME_LEN);
                    bestiary[bestiaryCount].effectivePotion[MAX_NAME_LEN - 1] = '\0';
                }
                bestiaryCount++;
                printf("New bestiary entry added: %s\n", enemy);
            }
        } else {
            if (isSign) {
                if (strlen(bestiary[index].effectiveSign) > 0 &&
                    strcasecmp(bestiary[index].effectiveSign, counter) == 0) {
                    printf("Already known effectiveness\n");
                } else {
                    strncpy(bestiary[index].effectiveSign, counter, MAX_NAME_LEN);
                    bestiary[index].effectiveSign[MAX_NAME_LEN - 1] = '\0';
                    printf("Bestiary entry updated: %s\n", enemy);
                }
            } else {
                if (strlen(bestiary[index].effectivePotion) > 0 &&
                    strcasecmp(bestiary[index].effectivePotion, counter) == 0) {
                    printf("Already known effectiveness\n");
                } else {
                    strncpy(bestiary[index].effectivePotion, counter, MAX_NAME_LEN);
                    bestiary[index].effectivePotion[MAX_NAME_LEN - 1] = '\0';
                    printf("Bestiary entry updated: %s\n", enemy);
                }
            }
        }
        return 1;
    }
    char* consistsPtr = strstr(learnPart, "consists of"); // Processing new potion formula: parse the potion name and its component ingredients, then add the new formula to the Formulas if it doesn't exist already.
    if (consistsPtr != NULL) {
        char* potionPtr = strstr(learnPart, "potion");
        if (!potionPtr) {
            printf("INVALID\n");
            return 1;
        }
        char potionName[MAX_NAME_LEN];
        int lenName = potionPtr - learnPart;
        if (lenName >= MAX_NAME_LEN)  
            lenName = MAX_NAME_LEN - 1;
        strncpy(potionName, learnPart, lenName);
        potionName[lenName] = '\0';
        trim(potionName);
        char* ingrList = consistsPtr + strlen("consists of");
        trim(ingrList);
        Item tempComponents[MAX_COMPONENTS];
        int compCount = 0;
        char* token = strtok(ingrList, ",");
        while (token != NULL && compCount < MAX_COMPONENTS) {
            trim(token);
            int qty;
            char ingrName[MAX_NAME_LEN];
            if (sscanf(token, "%d %s", &qty, ingrName) != 2 || qty <= 0) {
                printf("INVALID\n");
                return 1;
            }
            strncpy(tempComponents[compCount].name, ingrName, MAX_NAME_LEN);
            tempComponents[compCount].name[MAX_NAME_LEN - 1] = '\0';
            tempComponents[compCount].quantity = qty;
            compCount++;
            token = strtok(NULL, ",");
        }
        for (int i = 0; i < formula_count; i++) {
            if (strcasecmp(formulaBook[i].potionName, potionName) == 0) {
                printf("Already known formula\n");
                return 1;
            }
        }
        if (formula_count < MAX_FORMULAS) {
            strncpy(formulaBook[formula_count].potionName, potionName, MAX_NAME_LEN);
            formulaBook[formula_count].potionName[MAX_NAME_LEN - 1] = '\0';
            formulaBook[formula_count].componentCount = compCount;
            for (int i = 0; i < compCount; i++) {
                strncpy(formulaBook[formula_count].components[i].name, tempComponents[i].name, MAX_NAME_LEN);
                formulaBook[formula_count].components[i].name[MAX_NAME_LEN - 1] = '\0';
                formulaBook[formula_count].components[i].quantity = tempComponents[i].quantity;
            }
            formula_count++;
            printf("New alchemy formula obtained: %s\n", potionName);
            return 1;
        }
    }
    printf("INVALID\n");
    return 1;
}

//Encounter Action: "Geralt encounters a <monster>"
int processEncounter(char* input) {
// processEncounter: Simulates a monster encounter.
// Checks whether Geralt has an effective counter (either a sign or an available potion) for the enemy.
// If successful, consumes the potion (if applicable) and awards a trophy; otherwise, signals that Geralt is unprepared.

    if (strncmp(input, "Geralt encounters a ", 20) != 0)
        return 0;
    char* monster = input + 20;
    trim(monster);
    int len = strlen(monster);
   
    int index = -1;
    for (int i = 0; i < bestiaryCount; i++) {
        if (strcasecmp(bestiary[i].monsterName, monster) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf("Geralt is unprepared and barely escapes with his life\n");
        return 1;
    }
    int hasEffective = 0;
    if (strlen(bestiary[index].effectiveSign) > 0)
        hasEffective = 1;
    if (strlen(bestiary[index].effectivePotion) > 0 &&
        hasEnoughItem(bestiary[index].effectivePotion, 1))
        hasEffective = 1;
    if (!hasEffective) {
        printf("Geralt is unprepared and barely escapes with his life\n");
        return 1;
    }
    if (strlen(bestiary[index].effectivePotion) > 0 &&
        hasEnoughItem(bestiary[index].effectivePotion, 1)) {
        removeItem(bestiary[index].effectivePotion, 1);
    }
    char trophyName[MAX_NAME_LEN];
    snprintf(trophyName, MAX_NAME_LEN, "%s trophy", monster);
    addItem(trophyName, 1);
    printf("Geralt defeats %s\n", monster);
    return 1;
}

//Main Input Loop//

int main() {
// main: Entry point of the program.
// This loop continuously reads and processes user input, dispatching commands to appropriate handlers.

    char input[MAX_INPUT_LEN];

    // Input loop with "» " prompt
    // Begin the input processing loop: the prompt ">> " is displayed and each user command is interpreted.

    while (1) {
        printf(">> ");
        fflush(stdout);
        if (!fgets(input, MAX_INPUT_LEN, stdin))
            break;
        input[strcspn(input, "\n")] = '\0'; // Remove newline

        if (endsWithQuestionMark(input)) { // If the input ends with '?', treat it as a query command.
            if (!processQuery(input))
                printf("INVALID\n");
        }        
        else if (strncmp(input, "Geralt loots", 12) == 0) { // "Geralt loots": Process loot acquisition and update the inventory.
            if (!processLoot(input))
                printf("INVALID\n");
        }
        else if (strncmp(input, "Geralt trades", 13) == 0) { // "Geralt trades": Process trade commands by swapping trophies for ingredients.
            if (!processTrade(input))
                printf("INVALID\n");
        }
        else if (strncmp(input, "Geralt brews", 12) == 0) { // "Geralt brews": Attempt to brew an item if the necessary potion formula exists and ingredients are available.
            if (!processBrew(input)) 
                printf("INVALID\n");
        }
        else if (strncmp(input, "Geralt learns", 13) == 0) { // "Geralt learns": Process learning commands for bestiary effectiveness or new potion formulas.
            if (!processLearn(input))
                printf("INVALID\n");
        }
        else if (strncmp(input, "Geralt encounters a", 19) == 0) { // "Geralt encounters a": Simulate an encounter with a monster and resolve combat outcomes.
            if (!processEncounter(input))
                printf("INVALID\n");
        }
        else if (strcasecmp(input, "Exit") == 0) {  // "Exit": Terminates the program.
            break;
        }
        else {
            printf("INVALID\n");
        }
    }
    return 0;
}