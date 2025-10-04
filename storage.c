#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "storage.h"

sqlite3* open_db() {
    sqlite3 *db;
    if (sqlite3_open(g_db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

int init(const char* db_path){
    if(db_path && db_path[0] != '\0'){
      strncpy(g_db_path,db_path,(sizeof(g_db_path) - 1));
      g_db_path[sizeof(g_db_path - 1)] = '\0';
    }
    
    sqlite3 *db = open_db();
    if (!db){
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL"
        ");";

    char *errmsg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "SQLite error: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    return 0;
}

   bool signup(const char *username, const char *password) {
    sqlite3 *db = open_db();
    if (!db){
        return false;
    }

    const char *sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Signup failed: %s\n", sqlite3_errmsg(db));
        ok = false;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}

   bool login(const char *username, const char *password) {
    sqlite3 *db = open_db();
    if (!db) return false;

    const char *sql = "SELECT password FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *stored_pass = sqlite3_column_text(stmt, 0);
        if (stored_pass && strcmp((const char *)stored_pass, password) == 0) {
            ok = true; 
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}