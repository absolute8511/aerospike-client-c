/*
 * Copyright 2008-2014 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/aerospike_index.h>
#include <aerospike/aerospike_query.h>
#include <aerospike/as_arraylist.h>
#include <aerospike/as_cluster.h>
#include <aerospike/as_error.h>
#include <aerospike/as_hashmap.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>
#include <aerospike/as_query.h>
#include <aerospike/as_map.h>
#include <aerospike/as_record.h>
#include <aerospike/as_status.h>
#include <aerospike/as_string.h>
#include <aerospike/as_stringmap.h>
#include <aerospike/as_val.h>
#include <aerospike/mod_lua.h>

#include "../test.h"
#include "../util/udf.h"
#include "../util/consumer_stream.h"

/******************************************************************************
 * GLOBAL VARS
 *****************************************************************************/

extern aerospike * as;
static bool server_has_double = false;

/******************************************************************************
 * MACROS
 *****************************************************************************/

#define LUA_FILE "src/test/lua/client_stream_simple.lua"
#define UDF_FILE "client_stream_simple"

#define NAMESPACE "test"
#define SET "query_foreach"

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static bool before(atf_suite * suite) {

	if ( ! udf_put(LUA_FILE) ) {
		error("failure while uploading: %s", LUA_FILE);
		return false;
	}

	if ( ! udf_exists(LUA_FILE) ) {
		error("lua file does not exist: %s", LUA_FILE);
		return false;
	}


	return true;
}

static bool after(atf_suite * suite) {
	
	if ( ! udf_remove(LUA_FILE) ) {
		error("failure while removing: %s", LUA_FILE);
		return false;
	}

	return true;
}

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( query_foreach_exists, UDF_FILE" exists" ) {
	assert_true( udf_exists(LUA_FILE) );
}

/**
 * Creates 100 records and 4 indices.
 *
 * Records are structured as:
 *      {a: String, b: Integer, c: Integer, d: Integer, e: Integer}
 *
 * The key is "a-b-c-d-e"
 *
 * The values are:
 *      a = "abc"
 *      b = 100
 *      c = <current index>
 *      d = c % 10
 *      e = b + (c + 1) * (d + 1) / 2
 */
TEST( query_foreach_create, "create 100 records and 4 indices" ) {

	as_error err;
	as_error_reset(&err);

	int n_recs = 100;
	
	as_status status;
	as_index_task task;

	// create index on "a"
	
	status = aerospike_index_create(as, &err, &task, NULL, NAMESPACE, SET, "a", "idx_test_a", AS_INDEX_STRING);
	if ( status == AEROSPIKE_OK ) {
		aerospike_index_create_wait(&err, &task, 0);
	}
	else {
		info("error(%d): %s", err.code, err.message);
	}

	// create index on "b"

	status = aerospike_index_create(as, &err, &task, NULL, NAMESPACE, SET, "b", "idx_test_b", AS_INDEX_NUMERIC);
	if ( status == AEROSPIKE_OK ) {
		aerospike_index_create_wait(&err, &task, 0);
	}
	else {
		info("error(%d): %s", err.code, err.message);
	}

	// create index on "c"

	status = aerospike_index_create(as, &err, &task, NULL, NAMESPACE, SET, "c", "idx_test_c", AS_INDEX_NUMERIC);
	if ( status == AEROSPIKE_OK ) {
		aerospike_index_create_wait(&err, &task, 0);
	}
	else {
		info("error(%d): %s", err.code, err.message);
	}

	// create index on "d"
	status = aerospike_index_create(as, &err, &task, NULL, NAMESPACE, SET, "d", "idx_test_d", AS_INDEX_NUMERIC);
	if ( status == AEROSPIKE_OK ) {
		aerospike_index_create_wait(&err, &task, 0);
	}
	else {
		info("error(%d): %s", err.code, err.message);
	}

/* Uncomment once sindex on cdt is avaialable at server side.
	// create complex index on "x"

	status = aerospike_index_create_complex(as, &err, 0, NULL, NAMESPACE, SET, "x", "idx_test_x", AS_INDEX_TYPE_LIST, AS_INDEX_STRING);
    if (status != AEROSPIKE_OK) {
        info("error(%d): %s", err.code, err.message);
    }

	// create complex index on "y"

	status = aerospike_index_create_complex(as, &err, 0, NULL, NAMESPACE, SET, "y", "idx_test_y", AS_INDEX_TYPE_MAPKEYS, AS_INDEX_STRING);
    if (status != AEROSPIKE_OK) {
        info("error(%d): %s", err.code, err.message);
    }

	// create complex index on "y"

	status = aerospike_index_create_complex(as, &err, 0, NULL, NAMESPACE, SET, "y", "idx_test_y1", AS_INDEX_TYPE_MAPVALUES, AS_INDEX_STRING);
    if (status != AEROSPIKE_OK) {
        info("error(%d): %s", err.code, err.message);
    }
*/
	// insert records
	for ( int i = 0; i < n_recs; i++ ) {

		char * 	a = "abc";
		int 	b = n_recs;
		int 	c = i;
		int 	d = i % 10;
		int 	e = b + (c + 1) * (d + 1) / 2;

		char keystr[64] = { '\0' };
		snprintf(keystr, 64, "%s-%d-%d-%d-%d", a, b, c, d, e);

		// Make list

		as_arraylist list;
		as_arraylist_init(&list, 3, 0);
		as_arraylist_append_str(&list, "x");
		as_arraylist_append_str(&list, "x");
		as_arraylist_append_str(&list, "x");

		// Make map

		as_hashmap map;
		as_hashmap_init(&map, 1);
		as_stringmap_set_str((as_map *) &map, "y", "y");


		as_record r;
		as_record_init(&r, 7);
		as_record_set_str(&r, 	"a", a);
		as_record_set_int64(&r, "b", b);
		as_record_set_int64(&r, "c", c);
		as_record_set_int64(&r, "d", d);
		as_record_set_int64(&r, "e", e);
		as_record_set_list(&r, "x", (as_list *) &list);
		as_record_set_map(&r, "y", (as_map *) &map);


		as_key key;
		as_key_init(&key, NAMESPACE, SET, keystr);

		aerospike_key_put(as, &err, NULL, &key, &r);

		assert_int_eq( err.code, AEROSPIKE_OK);


		as_record_destroy(&r);

		as_record *r1 = NULL;

		aerospike_key_exists(as, &err, NULL, &key, &r1);

		as_key_destroy(&key);	
		assert_int_eq( err.code, AEROSPIKE_OK );
		assert_not_null( r1 );

		as_record_destroy(r1);
	}
}

static bool query_foreach_1_callback(const as_val * v, void * udata) {
	int * count = (int *) udata;
	if ( v == NULL ) {
		info("count: %d", (*count));
	}
	else {
		*count += 1;
	}
	return true;
}

TEST( query_foreach_1, "count(*) where a == 'abc' (non-aggregating)" ) {

	as_error err;
	as_error_reset(&err);

	int count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_select_inita(&q, 1);
	as_query_select(&q, "c");
	
	as_query_where_inita(&q, 1);
	as_query_where(&q, "a", as_string_equals("abc"));
	
	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_1_callback, &count);

	assert_int_eq( err.code, 0 );
	assert_int_eq( count, 100 );

	as_query_destroy(&q);
}

static bool query_foreach_2_callback(const as_val * v, void * udata) {
	if ( v != NULL ) {
		as_integer * i = as_integer_fromval(v);
		if ( i ) {
			int64_t * count = (int64_t *) udata;
			*count = i ? as_integer_toint(i) : 0;
		}
	}
	return true;
}

TEST( query_foreach_2, "count(*) where a == 'abc' (aggregating)" ) {

	as_error err;
	as_error_reset(&err);

	int64_t count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "a", as_string_equals("abc"));

	as_query_apply(&q, UDF_FILE, "count", NULL);
	
	if ( aerospike_query_foreach(as, &err, NULL, &q, query_foreach_2_callback, &count) != AEROSPIKE_OK ) {
		error("%s (%d) [%s:%d]", err.message, err.code, err.file, err.line);
	}

	info("count: %d",count);
	
	assert_int_eq( err.code, 0 );
	assert_int_eq( count, 100 );

	as_query_destroy(&q);
}


static bool query_foreach_3_callback(const as_val * v, void * udata) {
	if ( v != NULL ) {
		as_integer * result = as_integer_fromval(v);
		if ( result != NULL ) {
			int64_t * value = (int64_t *) udata;
			*value = as_integer_get(result);
		}
	}
	return true;
}

TEST( query_foreach_3, "sum(e) where a == 'abc'" ) {
	
	as_error err;
	as_error_reset(&err);

	int64_t value = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "a", as_string_equals("abc"));

	as_query_apply(&q, UDF_FILE, "sum", NULL);

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_3_callback, &value);

	if ( err.code != AEROSPIKE_OK ) {
		 fprintf(stderr, "error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
	}

	info("value: %ld", value);


	assert_int_eq( err.code, AEROSPIKE_OK );
	assert_int_eq( value, 24275 );

	as_query_destroy(&q);
}

static bool query_foreach_4_callback(const as_val * v, void * udata) {
	if ( v != NULL ) {
		as_integer * result = as_integer_fromval(v);
		if ( result != NULL ) {
			int64_t * value = (int64_t *) udata;
			*value = as_integer_get(result);
		}
	}
	return true;
}

TEST( query_foreach_4, "sum(d) where b == 100 and d == 1" ) {
	
	as_error err;
	as_error_reset(&err);

	int64_t value = 0;

	as_arraylist args;
	as_arraylist_init(&args, 2,0);
	as_arraylist_append_str(&args, "d");
	as_arraylist_append_int64(&args, 1);

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "b", as_integer_equals(100));

	as_query_apply(&q, UDF_FILE, "sum_on_match", (as_list *) &args);

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_4_callback, &value);

	if ( err.code != AEROSPIKE_OK ) {
		 fprintf(stderr, "error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
	}

	info("value: %ld", value);

	assert_int_eq( err.code, AEROSPIKE_OK );
	assert_int_eq( value, 10 );

	as_arraylist_destroy(&args);
	as_query_destroy(&q);
}

static bool query_foreach_count_callback(const as_val * v, void * udata) {
	int * count = (int *) udata;
	if ( v == NULL ) {
		info("count: %d", (*count));
	}
	else {
		*count += 1;
	}
	return true;
}

TEST( query_foreach_5, "IN LIST count(*) where x contains 'x'" ) {
	
	as_error err;
	as_error_reset(&err);

	int count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "x", as_contains(LIST, STRING, "x"));

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_count_callback, &count);

	if ( err.code != AEROSPIKE_OK ) {
		 fprintf(stderr, "error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
	}

	assert_int_eq( err.code, AEROSPIKE_OK );
	assert_int_eq( count, 100 );

	as_query_destroy(&q);
}

TEST( query_foreach_6, "IN MAPKEYS count(*) where y contains 'y'" ) {
	
	as_error err;
	as_error_reset(&err);

	int count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "y", as_contains(MAPKEYS, STRING, "y"));

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_count_callback, &count);

	if ( err.code != AEROSPIKE_OK ) {
		 fprintf(stderr, "error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
	}

	assert_int_eq( err.code, AEROSPIKE_OK );
	assert_int_eq( count, 100 );

	as_query_destroy(&q);
}

TEST( query_foreach_7, "IN MAPVALUES count(*) where y contains 'y'" ) {

	as_error err;
	as_error_reset(&err);

	int count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "y", as_contains(MAPVALUES, STRING, "y"));

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_count_callback, &count);

	if ( err.code != AEROSPIKE_OK ) {
		 fprintf(stderr, "error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
	}

	assert_int_eq( err.code, AEROSPIKE_OK );
	assert_int_eq( count, 100 );

	as_query_destroy(&q);
}

static bool query_quit_early_callback(const as_val * v, void * udata) {
	if (v) {
		int64_t* count = (int64_t*)udata;
		(*count)++;
	}
	return false;
}

TEST( query_quit_early, "normal query and quit early" ) {
	
	as_nodes* nodes = as_nodes_reserve(as->cluster);
	uint32_t nodes_size = nodes->size;
	as_nodes_release(nodes);

	as_error err;
	as_error_reset(&err);
	
	int64_t count = 0;
	
	as_query q;
	as_query_init(&q, NAMESPACE, SET);
	
	as_query_where_inita(&q, 1);
	as_query_where(&q, "a", as_string_equals("abc"));
	
	if ( aerospike_query_foreach(as, &err, NULL, &q, query_quit_early_callback, &count) != AEROSPIKE_OK ) {
		error("%s (%d) [%s:%d]", err.message, err.code, err.file, err.line);
	}
	
	info("count: %d",count);
	
	assert_int_eq( err.code, 0 );
	assert_true( count <= nodes_size );
	
	as_query_destroy(&q);
}

TEST( query_agg_quit_early, "aggregation and quit early" ) {
	
	as_nodes* nodes = as_nodes_reserve(as->cluster);
	uint32_t nodes_size = nodes->size;
	as_nodes_release(nodes);

	as_error err;
	as_error_reset(&err);
	
	int64_t count = 0;
	
	as_query q;
	as_query_init(&q, NAMESPACE, SET);
	
	as_query_where_inita(&q, 1);
	as_query_where(&q, "a", as_string_equals("abc"));
	
	as_query_apply(&q, UDF_FILE, "filter_passthrough", NULL);
	
	if ( aerospike_query_foreach(as, &err, NULL, &q, query_quit_early_callback, &count) != AEROSPIKE_OK ) {
		error("%s (%d) [%s:%d]", err.message, err.code, err.file, err.line);
	}
	
	info("count: %d",count);
	
	assert_int_eq( err.code, 0 );
	assert_true( count <= nodes_size );
	
	as_query_destroy(&q);
}

static bool query_quit_early_bytes_callback(const as_val * v, void * udata) {
	if (v) {
		as_bytes * bval = as_bytes_fromval(v);

		if (bval) {
			int64_t* byte_count = (int64_t*)udata;
			(*byte_count) += as_bytes_size(bval);
		}
	}
	return false;
}

TEST( query_filter_map_bytes, "return bytes from a mapper" ) {

	as_error err;
	as_error_reset(&err);

	int64_t byte_count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "a", as_string_equals("abc"));

	as_query_apply(&q, UDF_FILE, "filter_passthrough_digest", NULL);

	if ( aerospike_query_foreach(as, &err, NULL, &q, query_quit_early_bytes_callback, &byte_count) != AEROSPIKE_OK ) {
		error("%s (%d) [%s:%d]", err.message, err.code, err.file, err.line);
	}

	info("byte count: %d",byte_count);

	assert_int_eq( err.code, 0 );
	assert_int_eq( byte_count, 20 ); // one digest

	as_query_destroy(&q);
}

TEST( query_foreach_nullset, "test null-set behavior" ) {

	as_error err;
	as_error_reset(&err);

	as_status status;

	char *setname = NULL;
	as_index_task task;

	status = aerospike_index_create(as, &err, &task, NULL, NAMESPACE, setname, "NUMERIC", "idx2", AS_INDEX_NUMERIC);
	if ( status != AEROSPIKE_OK ) {
		info("error(%d): %s", err.code, err.message);
	} else {
		aerospike_index_create_wait(&err, &task, 0);
	}

	as_record r;
	as_record_init(&r, 2);
	as_record_set_int64(&r, "NUMERIC", 1);
	as_record_set_str(&r, 	"bn_STRING", "2");
	as_record_set_int64(&r, "bn2", 3);

	as_key key;
	as_key_init(&key, NAMESPACE, setname, "keyindex-nullset");

	aerospike_key_put(as, &err, NULL, &key, &r);
	assert_int_eq( err.code, AEROSPIKE_OK);

	as_record_destroy(&r);
	as_key_destroy(&key);

	int64_t count = 0;

	as_query q;
	as_query_init(&q, NAMESPACE, setname);

	as_query_where_inita(&q, 1);
	as_query_where(&q, "NUMERIC", as_integer_equals(1));

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_count_callback, &count);
	info("my count: %d",count);
	assert_true(count == 1);

}

static bool query_foreach_double_callback(const as_val * v, void * udata) {
	
	if (v) {
		as_record* rec = as_record_fromval(v);
		double * sum = (double *) udata;
		*sum += as_record_get_double(rec,"double_bin", 0.0);
	}
    return true;
}

TEST( query_foreach_double, "test query on double behavior" ) {

	as_error err;
	as_error_reset(&err);

	int n_recs = 1000, n_q_recs = 99;
	char *int_bin = "int_bin";
	char *double_bin = "double_bin";

	as_status status;

	as_index_task task;

	//create index on "int_bin"
	status = aerospike_index_create(as, &err, &task, NULL, NAMESPACE, SET, int_bin, "idx_test_int_bin", AS_INDEX_NUMERIC);
	if ( status == AEROSPIKE_OK ) {
		aerospike_index_create_wait(&err, &task, 0);
	}
	else {
		info("error(%d): %s", err.code, err.message);
	}

	as_record r;
	as_record_init(&r, 2);
	// insert records
	for ( int i = 1; i <= n_recs; i++ ) {
		as_key key;
		as_key_init_int64(&key, NAMESPACE, SET, (int64_t)i);

		as_record_set_int64(&r, int_bin, i);
		as_record_set_double(&r, double_bin, i/(double)10);

		aerospike_key_put(as, &err, NULL, &key, &r);
	}
	as_record_destroy(&r);

	as_query q;
	as_query_init(&q, NAMESPACE, SET);

	as_query_select_inita(&q, 1);
	as_query_select(&q, double_bin);

	as_query_where_inita(&q, 1);
	as_query_where(&q, int_bin, as_integer_range(0, n_q_recs));
	
	double expected_sum = 0;
	double recieved_sum = 0;

	aerospike_query_foreach(as, &err, NULL, &q, query_foreach_double_callback, &recieved_sum);

	for ( int j = 1; j <= n_q_recs; j++ ) {
		expected_sum += j/(double)10;
	}

	assert_int_eq( err.code, 0 );
	assert_double_eq( recieved_sum, expected_sum );

	as_query_destroy(&q);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( query_foreach, "aerospike_query_foreach tests" ) {

	server_has_double = aerospike_has_double(as);

	suite_before( before );
	suite_after( after   );
	
	suite_add( query_foreach_create );
	suite_add( query_foreach_1 );
	suite_add( query_foreach_2 );
	suite_add( query_foreach_3 );
	suite_add( query_foreach_4 );
/* Uncomment once sindex on cdt feature is available at server side.
	suite_add( query_foreach_5 );
	suite_add( query_foreach_6 );
	suite_add( query_foreach_7 );
*/
	suite_add( query_quit_early );
	suite_add( query_agg_quit_early );
	suite_add( query_filter_map_bytes );
	suite_add( query_foreach_nullset );
	if (server_has_double) {
		suite_add( query_foreach_double );
	}
}
