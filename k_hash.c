#include "k_hash.h"

#include <assert.h>
#include <string.h>

static int str_hash(const char* str_val,int max_size)
{
	return strlen(str_val) % max_size;
}

static int int_hash(const int*  int_val, int max_size)
{
	return (*int_val) % max_size;
}

int str_compare(const char* left, const char* right)
{
	return strcmp(left,right);
}

int int_compare(const int* left, const int* right)
{
	return *left - *right;
}


k_status_t k_hash_init_string(k_hash_table_t* hash_table, k_mpool_t* pool,
	k_size_t hash_arr_size, k_getkey_t fn_getkey)
{
	return k_hash_init(hash_table, pool, hash_arr_size, fn_getkey, str_hash, str_compare);
}

k_status_t k_hash_init_integer(k_hash_table_t* hash_table, k_mpool_t* pool,
	k_size_t hash_arr_size, k_getkey_t fn_getkey)
{
	return k_hash_init(hash_table, pool, hash_arr_size, fn_getkey, int_hash, int_compare);
}

k_status_t k_hash_entry_init(k_hash_entry_t* hash_entry)
{
	hash_entry->parent = K_NULL;
	k_list_init((k_list_t*)hash_entry);
	return K_SUCCESS;
}

k_status_t k_hash_init(k_hash_table_t* hash_table, k_mpool_t* pool,
	k_size_t buckets_count, k_getkey_t fn_getkey,
	k_gethash_t fn_gethash, k_compare_t fn_compare)
{
	int buckets_size = buckets_count * sizeof(void*);
	hash_table->pool = pool;
	hash_table->buckets_count = buckets_count;
	hash_table->buckets = k_mpool_malloc(pool, buckets_count * sizeof(void*));
	memset(hash_table->buckets, 0, hash_table->buckets_count * sizeof(void*));
	k_list_init((k_list_t*)&hash_table->values);
	hash_table->gethash = fn_gethash;
	hash_table->getkey = fn_getkey;
	hash_table->compare = fn_compare;
	hash_table->val_count = 0;
	return K_SUCCESS;
}


k_status_t k_hash_destroy(k_hash_table_t* hash_table)
{
	return K_SUCCESS;
}

static void k_hash_bucket_init(k_hash_bucket_t* bucket, k_size_t hash_code)
{
	bucket->hash_code = hash_code;
	k_list_init((k_list_t*)bucket);
	k_list_init((k_list_t*)&bucket->hash_entries);
}

k_status_t k_hash_put(k_hash_table_t* hash_table, k_hash_entry_t* val)
{
	void* hash_key = hash_table->getkey(val);
	k_size_t   hash_code = hash_table->gethash(hash_key,hash_table->buckets_count);
	assert(hash_code < hash_table->buckets_count);
	if (k_hash_get(hash_table, hash_key) != K_NULL) {
		return K_ERROR;
	}
	k_hash_bucket_t* bucket = hash_table->buckets[hash_code];
	if (bucket == K_NULL) {
		bucket = (k_hash_bucket_t*)k_mpool_malloc(
			hash_table->pool, sizeof(k_hash_bucket_t));
		k_hash_bucket_init(bucket, hash_code);
		
		bucket->hash_entries.parent = K_NULL;
		val->parent = bucket;

		hash_table->buckets[hash_code] = bucket;
		
	}
	k_list_insert_before((k_list_t*)&bucket->hash_entries, (k_list_t*)val);
	k_hash_entry_t* entries = &bucket->hash_entries;
	hash_table->val_count++;
	assert(val->next != val);
	assert(val->prev != val);
	return K_SUCCESS;
}

k_hash_entry_t* k_hash_get(k_hash_table_t* hash_table, const void* key)
{
	k_size_t   hash_code = hash_table->gethash(key,hash_table->buckets_count);
	k_hash_bucket_t* bucket = hash_table->buckets[hash_code];
	if (bucket == K_NULL) {
		return K_NULL;
	}

	k_hash_entry_t* entries = &bucket->hash_entries;

	k_list_t* list_next = (k_list_t*)entries->next;
	while (list_next !=(k_list_t*)entries)
	{
		assert(list_next->next != list_next);
		void* this_key = hash_table->getkey(list_next);
		if (hash_table->compare(key, this_key) == 0) {
			return (k_hash_entry_t*)list_next;
		}
		list_next = list_next->next;
	}

	return K_NULL;
}

k_hash_entry_t* k_hash_remove(k_hash_table_t* hash_table, const void* key)
{
	k_hash_entry_t* entry = k_hash_get(hash_table, key);
	if (entry == K_NULL) {
		return K_NULL;
	}
	hash_table->val_count++;
	return (k_hash_entry_t*)k_list_remove((k_list_t*)entry);
}