//
// Created by mistershift on 3/30/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//Import local files
#include "../../include/logging.h"
#include "../../include/server/hash_table.h"

// Fonction de hachage djb2
unsigned int hash(const char *id, size_t size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *id++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % size;
}

// Initialiser la table de hachage
void init_client_table(ClientHashTable *hashTable) {
    hashTable->size = INITIAL_HASH_SIZE;
    hashTable->count = 0;
    hashTable->table = (Client **)calloc(hashTable->size, sizeof(Client *));
    if (!hashTable->table) {
        output_log("Erreur d'allocation mémoire dans init_client_table()\n", LOG_ERROR, LOG_TO_ALL);
        exit(EXIT_FAILURE);
    }
}

// Redimensionner la table de hachage
void resize_client_table(ClientHashTable *hashTable) {
    size_t newSize = hashTable->size * 2;
    Client **newTable = (Client **)calloc(newSize, sizeof(Client *));
    if (!newTable) {
        output_log("Erreur d'allocation mémoire dans resize_client_table()\n", LOG_ERROR, LOG_TO_ALL);
        return;
    }

    for (size_t i = 0; i < hashTable->size; i++) {
        Client *current = hashTable->table[i];
        while (current) {
            Client *next = current->next;
            unsigned int newIndex = hash(current->id, newSize);
            current->next = newTable[newIndex];
            newTable[newIndex] = current;
            current = next;
        }
    }

    free(hashTable->table);
    hashTable->table = newTable;
    hashTable->size = newSize;
}

// Ajouter un client à la table de hachage
void add_client(ClientHashTable *hashTable, const char *id, int socket, bool connected) {
    if ((double)hashTable->count / hashTable->size > LOAD_FACTOR) {
        resize_client_table(hashTable);
    }

    unsigned int index = hash(id, hashTable->size);
    Client *newClient = (Client *)malloc(sizeof(Client));
    if (!newClient) {
        output_log("Erreur d'allocation mémoire dans add_client()\n", LOG_ERROR, LOG_TO_ALL);
        return;
    }
    strcpy(newClient->id, id);
    newClient->socket = socket;
    newClient->connected = connected;  // Initialisation de l'état de connexion
    newClient->next = hashTable->table[index];
    hashTable->table[index] = newClient;
    hashTable->count++;
}

// Rechercher un client dans la table de hachage
Client *find_client(ClientHashTable *hashTable, const char *id) {
    unsigned int index = hash(id, hashTable->size);
    Client *current = hashTable->table[index];
    while (current) {
        if (strcmp(current->id, id) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Supprimer un client de la table de hachage
void remove_client(ClientHashTable *hashTable, const char *id) {
    unsigned int index = hash(id, hashTable->size);
    Client *current = hashTable->table[index];
    Client *prev = NULL;
    
    while (current) {
        if (strcmp(current->id, id) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                hashTable->table[index] = current->next;
            }
            free(current);
            hashTable->count--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Libérer toute la mémoire allouée à la table de hachage
void free_client_table(ClientHashTable *hashTable) {
    for (size_t i = 0; i < hashTable->size; i++) {
        Client *current = hashTable->table[i];
        while (current) {
            Client *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(hashTable->table);
    hashTable->table = NULL;
    hashTable->size = 0;
    hashTable->count = 0;
}

void print_client_table(ClientHashTable *hashTable) {
    printf("Hash table size: %zu\n", hashTable->size);
    printf("Total clients: %zu\n", hashTable->count);
    
    for (size_t i = 0; i < hashTable->size; i++) {
        Client *current = hashTable->table[i];
        if (current != NULL) {
            printf("Index %zu: ", i);
            while (current) {
                printf("[ID: %s, Socket: %d,State : %d] -> ", current->id, current->socket,current->connected);
                current = current->next;
            }
            printf("NULL\n");
        }
    }
}
