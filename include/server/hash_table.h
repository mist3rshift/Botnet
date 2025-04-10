//
// Created by mistershift on 3/30/2025.
//
#ifndef TABLE_HASH_H
#define TABLE_HASH_H
#include <stdbool.h>
#include <string.h>
#define INITIAL_HASH_SIZE 128
#define LOAD_FACTOR 0.75  // Seuil de redimensionnement

typedef enum {
    LISTENING,   // Le client écoute activement le serveur
    ACTIVE,      // Le client effectue une tâche demandée par le serveur
    UNREACHABLE  // La connexion est fermée, le client n'écoute plus
} ClientState;

// Structure représentant un client
typedef struct Client {
    char id[64];  // Identifiant unique du client
    int socket;   // Socket du client
    ClientState state; // Etat du client
    struct Client *next; // Gestion des collisions (chaînage)
} Client;

// Table de hachage des clients
typedef struct {
    Client **table;
    size_t size;
    size_t count;
} ClientHashTable;

extern ClientHashTable hash_table; // Declare global (ro)

//Fonctions declarations
unsigned int hash(const char *id, size_t size);

void init_client_table(ClientHashTable *hashTable);

void resize_client_table(ClientHashTable *hashTable);

void add_client(ClientHashTable *hashTable, const char *id, int socket, ClientState state);

Client *find_client(const ClientHashTable *hashTable, const char *id);

Client *find_client_by_socket(const ClientHashTable *hashTable, const int socket);

void remove_client(ClientHashTable *hashTable, const char *id);

void free_client_table(ClientHashTable *hashTable);

void print_client_table(const ClientHashTable *hashTable);


#endif