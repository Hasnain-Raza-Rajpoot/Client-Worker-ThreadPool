#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "storage.h"

#define DEFAULT_QUOTA_BYTES (100 * 1024 * 1024)

sqlite3* open_db() {
    sqlite3 *db;
    if (sqlite3_open(g_db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

int init_db(const char* db_path){
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
        "username TEXT UNIQUE NOT NULL CHECK (length(username) <= 256),"
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
    if (init_quotas_table() != 0) {
            return -1;
    }
    return 0;
}
int init_quotas_table() {
    sqlite3 *db = open_db();
    if (!db) return -1;

    const char *sql =
        "CREATE TABLE IF NOT EXISTS quotas ("
        "user_id INTEGER PRIMARY KEY,"
        "quota_bytes INTEGER DEFAULT 104857600,"  
        "used_bytes INTEGER DEFAULT 0,"
        "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    char *errmsg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "SQLite error creating quotas table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    printf("Quotas table initialized\n");
    return 0;
}

bool signup(const char *username, const char *password) {
    //length check
    if (strlen(username) == 0 || strlen(username) > 256) {
        fprintf(stderr, "Signup failed: Username must be 1–256 characters.\n");
        return false;
    }

    sqlite3 *db = open_db();
    if (!db) return false;

    //check if username already exists
    const char *check_sql = "SELECT 1 FROM users WHERE username = ?;";
    sqlite3_stmt *check_stmt;

    if (sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(check_stmt, 1, username, -1, SQLITE_STATIC);

    bool exists = (sqlite3_step(check_stmt) == SQLITE_ROW);
    sqlite3_finalize(check_stmt);

    if (exists) {
        fprintf(stderr, "Signup failed: Username '%s' already exists. Choose a different one.\n", username);
        sqlite3_close(db);
        return false;
    }
    const char *insert_sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
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
    if (ok) {
        int user_id = get_user_id(username);
        if (user_id != -1) {
            const char *quota_sql = "INSERT INTO quotas (user_id, quota_bytes, used_bytes) VALUES (?, ?, 0);";
            sqlite3_stmt *quota_stmt;
            
            if (sqlite3_prepare_v2(db, quota_sql, -1, &quota_stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(quota_stmt, 1, user_id);
                sqlite3_bind_int64(quota_stmt, 2, DEFAULT_QUOTA_BYTES);
                
                if (sqlite3_step(quota_stmt) == SQLITE_DONE) {
                    printf("Quota entry created for user %d (limit: %ld bytes)\n", 
                           user_id, DEFAULT_QUOTA_BYTES);
                } else {
                    fprintf(stderr, "Failed to create quota entry: %s\n", sqlite3_errmsg(db));
                }
                
                sqlite3_finalize(quota_stmt);
            }
        }
    }
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

int get_user_id(const char *username) {
    sqlite3 *db = open_db();
    if (!db) return -1;

    const char *sql = "SELECT id FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int user_id = -1;  
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return user_id;
}
long get_user_quota(int user_id) {
    sqlite3 *db = open_db();
    if (!db) return -1;

    const char *sql = "SELECT quota_bytes FROM quotas WHERE user_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    long quota = DEFAULT_QUOTA_BYTES; 
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        quota = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return quota;
}
long get_used_space(int user_id) {
    sqlite3 *db = open_db();
    if (!db) return -1;

    const char *sql = "SELECT used_bytes FROM quotas WHERE user_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    long used = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        used = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return used;
}
bool update_used_space(int user_id, long delta) {
    sqlite3 *db = open_db();
    if (!db) return false;
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    const char *sql = "UPDATE quotas SET used_bytes = used_bytes + ? WHERE user_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, delta);
    sqlite3_bind_int(stmt, 2, user_id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    
    sqlite3_finalize(stmt);

    if (ok) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
        printf("Updated used space for user %d: delta=%ld bytes\n", user_id, delta);
    } else {
        fprintf(stderr, "Failed to update used space: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }

    sqlite3_close(db);
    return ok;
}
bool check_quota(int user_id, long file_size) {
    long quota = get_user_quota(user_id);
    long used = get_used_space(user_id);

    if (quota == -1 || used == -1) {
        fprintf(stderr, "Failed to check quota for user %d\n", user_id);
        return false;
    }

    long available = quota - used;
    
    printf("[QUOTA] User %d: used=%ld, quota=%ld, available=%ld, requested=%ld\n",
           user_id, used, quota, available, file_size);

    if (file_size > available) {
        printf("[QUOTA] User %d quota exceeded! Need %ld bytes, only %ld available\n",
               user_id, file_size, available);
        return false;
    }

    return true;
}
void set_user_quota(int user_id, long quota_bytes) {
    sqlite3 *db = open_db();
    if (!db) return;

    const char *sql = "UPDATE quotas SET quota_bytes = ? WHERE user_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int64(stmt, 1, quota_bytes);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("Set quota for user %d to %ld bytes\n", user_id, quota_bytes);
    } else {
        fprintf(stderr, "Failed to set quota: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}