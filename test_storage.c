// test_storage.c
#include <stdio.h>
#include <string.h>
#include "storage.h"

int main() {
    const char *db_path = "test.db";

    // Initialize the database
    if (init(db_path) != 0) {
        printf("Failed to initialize database.\n");
        return 1;
    }
    printf("Database initialized successfully.\n");

    // Test signup
    const char *username = "user1";
    const char *password = "pass123";

    if (signup(username, password)) {
        printf("Signup successful for user: %s\n", username);
    } else {
        printf("Signup failed for user: %s\n", username);
    }

    // Test login with correct credentials
    if (login(username, password)) {
        printf("Login successful for user: %s\n", username);
    } else {
        printf("Login failed for user: %s\n", username);
    }

    // Test login with incorrect password
    if (login(username, "wrongpass")) {
        printf("Login should not succeed with wrong password!\n");
    } else {
        printf("Login correctly failed with wrong password.\n");
    }

    return 0;
}
