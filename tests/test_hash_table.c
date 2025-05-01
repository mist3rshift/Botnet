//
// Created by mistershift on 3/30/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../include/server/hash_table.h"

static void test_init_client_table(void **state) {
    (void)state; // Unused
    ClientHashTable table;
    init_client_table(&table);

    free_client_table(&table);
}

static void test_add_find_client(void **state) {
    (void)state; // Unused
    ClientHashTable table;
    init_client_table(&table);

    add_client(&table, "client1", 1,LISTENING);
    add_client(&table, "client2", 2,ACTIVE);
    add_client(&table, "client3", 3,UNREACHABLE);
    //print_client_table(&table);
    Client *c1 = find_client(&table, "client1");
    Client *c2 = find_client(&table, "client2");
    Client *c3 = find_client_by_socket(&table, 3);

    free_client_table(&table);
}

static void test_remove_client(void **state) {
    (void)state; // Unused
    ClientHashTable table;
    init_client_table(&table);

    add_client(&table, "client1", 1,LISTENING);
    remove_client(&table, "client1");

    Client *c1 = find_client(&table, "client1");

    free_client_table(&table);
}

static void test_resize_table(void **state) {
    (void)state; // Unused
    ClientHashTable table;
    init_client_table(&table);

    // Ajout de clients pour forcer le redimensionnement
    for (int i = 0; i < 200; i++) {
        char id[10];
        sprintf(id, "client%d", i);
        add_client(&table, id, i,LISTENING);
    }
    Client *c150 = find_client(&table, "client150");

    free_client_table(&table);
}

static void test_free_client_table(void** state) {
    (void)state; // Unused
    ClientHashTable table;
    init_client_table(&table);

    // Ajouter des clients
    add_client(&table, "client1", 1,LISTENING);
    add_client(&table, "client2", 2,ACTIVE);

    // Vérification avant libération
    Client *c1_before = find_client(&table, "client1");
    Client *c2_before = find_client(&table, "client2");
    //printf("Before free: client1 %s, client2 %s\n", 
    //       (c1_before != NULL) ? "FOUND" : "NOT FOUND", 
    //       (c2_before != NULL) ? "FOUND" : "NOT FOUND");

    // Libérer la mémoire de la table
    free_client_table(&table);

    // Après avoir libéré la table, vérifier que la mémoire est bien libérée
    // Ne pas accéder à la table, car la mémoire a été libérée.
    // Les pointeurs sont invalides après free_client_table, donc il ne faut pas les utiliser.

    // Simplement vérifier que la table a été libérée correctement (compte des clients).
}


int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init_client_table),
        cmocka_unit_test(test_add_find_client),
        cmocka_unit_test(test_remove_client),
        cmocka_unit_test(test_resize_table),
        cmocka_unit_test(test_free_client_table),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
