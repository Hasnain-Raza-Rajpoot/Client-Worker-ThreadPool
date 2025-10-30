#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include <stdbool.h>

extern char g_db_path[256];

sqlite3* open_db();

int init_db(const char* db_path);

bool signup(const char *username, const char *password);

bool login(const char *username, const char *password);

int get_user_id(const char *username);

int init_quotas_table();

long get_user_quota(int user_id);

long get_used_space(int user_id);

bool update_used_space(int user_id, long delta);

bool check_quota(int user_id, long file_size);

void set_user_quota(int user_id, long quota_bytes);


#endif
