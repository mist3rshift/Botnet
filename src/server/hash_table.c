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

ClientHashTable hash_table;

// Fonction de hachage djb2
unsigned int hash(const char *id, const size_t size) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char) *id++)) {
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
void add_client(ClientHashTable *hash_table, const char *id, int socket, ClientState state) {
    // Vérifiez si le client existe déjà
    Client *existing_client = find_client_by_socket(hash_table, socket);
    if (existing_client) {
        // Mettre à jour l'état et le socket
        existing_client->socket = socket;
        existing_client->state = state;

        return;
    }

    // Si le client n'existe pas, ajoutez-le
    Client *new_client = malloc(sizeof(Client));
    strncpy(new_client->id, id, sizeof(new_client->id) - 1);
    new_client->id[sizeof(new_client->id) - 1] = '\0';
    new_client->socket = socket;
    new_client->state = state;
    new_client->next = NULL;

    // Ajoutez le client à la table de hachage
    unsigned int index = hash(id, hash_table->size);
    new_client->next = hash_table->table[index];
    hash_table->table[index] = new_client;
}

// Rechercher un client dans la table de hachage
Client *find_client(const ClientHashTable *hashTable, const char *id) {
    unsigned int index = hash(id, hashTable->size);
    Client *client = hashTable->table[index];
    while (client != NULL) {
        if (strcmp(client->id, id) == 0) {
            return client;
        }
        client = client->next;
    }
    return NULL;
}

//Rechercher un client par socket dans la table de hachage
Client *find_client_by_socket(const ClientHashTable *hashTable, const int socket) {
    for (size_t i = 0; i < hashTable->size; i++) {
        Client *client = hashTable->table[i];
        while (client != NULL) {
            if (client->socket == socket) {
                return client;
            }
            client = client->next;
        }
    }
    return NULL;
}


// Supprimer un client de la table de hachage
void remove_client(ClientHashTable *hashTable, const char *id) {
    const unsigned int index = hash(id, hashTable->size);
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

void print_client_table(const ClientHashTable *hashTable) {
    printf("Hash table size: %zu\n", hashTable->size);
    printf("Total clients: %zu\n", hashTable->count);
    
    for (size_t i = 0; i < hashTable->size; i++) {
        Client *current = hashTable->table[i];
        if (current != NULL) {
            printf("Index %zu: ", i);
            while (current) {
                printf("[ID: %s, Socket: %d,State : %d] -> ", current->id, current->socket,current->state);
                current = current->next;
            }
            printf("NULL\n");
        }
    }
}

