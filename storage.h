#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

char g_db_path[256];

sqlite3* open_db();

int init(const char* db_path);

bool signup(const char *username, const char *password);

bool login(const char *username, const char *password);

#endif
