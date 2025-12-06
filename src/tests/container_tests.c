////////////////////////////////////////////////////////////////////////////////
// container_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <lox/ordered_table.h>

void print_list(List* list);

TestGroup container_tests;

static int test_list_remove_node()
{
    // Test 0-initialization as per Object/Mobile Entities
    List list = { 0 };
    init_list(&list);
    gc_protect(OBJ_VAL(&list));

    Value a = mock_str("a");
    Node* n_a = list_push_back(&list, a);

    Value b = mock_str("b");
    Node* n_b = list_push_back(&list, b);

    Value c = mock_str("c");
    Node* n_c = list_push_back(&list, c);

    Value d = mock_str("d");
    Node* n_d = list_push_back(&list, d);

    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { a, b, c, d }>");
    test_output_buffer = NIL_VAL;

    list_remove_node(&list, n_a);    
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { b, c, d }>");
    test_output_buffer = NIL_VAL;

    list_remove_node(&list, n_c);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { b, d }>");
    test_output_buffer = NIL_VAL;

    list_remove_node(&list, n_d);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { b }>");
    test_output_buffer = NIL_VAL;

    list_remove_node(&list, n_b);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { }>");
    test_output_buffer = NIL_VAL;

    gc_protect_clear();

    return 0;
}

static int test_list_remove_value()
{
    // Test 0-initialization as per Object/Mobile Entities
    List list = { 0 };
    init_list(&list);
    gc_protect(OBJ_VAL(&list));

    Value a = mock_str("a");
    list_push_back(&list, a);

    Value b = mock_str("b");
    list_push_back(&list, b);

    Value c = mock_str("c");
    list_push_back(&list, c);

    Value d = mock_str("d");
    list_push_back(&list, d);

    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { a, b, c, d }>");
    test_output_buffer = NIL_VAL;

    list_remove_value(&list, a);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { b, c, d }>");
    test_output_buffer = NIL_VAL;

    list_remove_value(&list, c);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { b, d }>");
    test_output_buffer = NIL_VAL;

    list_remove_value(&list, d);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { b }>");
    test_output_buffer = NIL_VAL;

    list_remove_value(&list, b);
    print_list(&list);

    ASSERT_OUTPUT_EQ("<list { }>");
    test_output_buffer = NIL_VAL;

    gc_protect_clear();

    return 0;
}

static int test_ordered_table_iteration()
{
    OrderedTable ordered = { 0 };
    ordered_table_init(&ordered);
    gc_protect(OBJ_VAL(&ordered.table));

    ordered_table_set_vnum(&ordered, 3001, mock_str("gamma"));
    ordered_table_set_vnum(&ordered, 1001, mock_str("alpha"));
    ordered_table_set_vnum(&ordered, 2001, mock_str("beta"));
    ordered_table_set_vnum(&ordered, 4001, mock_str("delta"));

    int32_t expected_keys[] = { 1001, 2001, 3001, 4001 };
    const char* expected_vals[] = { "alpha", "beta", "gamma", "delta" };

    OrderedTableIter iter = ordered_table_iter(&ordered);
    for (size_t i = 0; i < sizeof(expected_keys) / sizeof(expected_keys[0]); i++) {
        int32_t key = 0;
        Value value = NIL_VAL;
        ASSERT(ordered_table_iter_next(&iter, &key, &value));
        ASSERT(key == expected_keys[i]);
        ASSERT_LOX_STR_EQ(expected_vals[i], value);
    }
    ASSERT(!ordered_table_iter_next(&iter, NULL, NULL));

    ordered_table_free(&ordered);
    gc_protect_clear();
    return 0;
}

static int test_ordered_table_updates_and_deletes()
{
    OrderedTable ordered = { 0 };
    ordered_table_init(&ordered);
    gc_protect(OBJ_VAL(&ordered.table));

    Value v1 = mock_str("one");
    Value v2 = mock_str("two");
    Value v2_new = mock_str("two-new");

    ASSERT(ordered_table_set_vnum(&ordered, 1, v1));
    ASSERT(ordered_table_set_vnum(&ordered, 2, v2));
    ASSERT(!ordered_table_set_vnum(&ordered, 2, v2_new));

    Value fetched = NIL_VAL;
    ASSERT(ordered_table_get_vnum(&ordered, 2, &fetched));
    ASSERT(fetched == v2_new);

    ASSERT(ordered_table_delete_vnum(&ordered, 2));
    ASSERT(!ordered_table_contains_vnum(&ordered, 2));
    ASSERT(ordered_table_count(&ordered) == 1);
    ASSERT(ordered_table_delete_vnum(&ordered, 99) == false);

    ordered_table_free(&ordered);
    gc_protect_clear();
    return 0;
}

void register_container_tests()
{
#define REGISTER(n, f)  register_test(&container_tests, (n), (f))

    init_test_group(&container_tests, "CONTAINER TESTS");
    register_test_group(&container_tests);

    REGISTER("List: Remove Node", test_list_remove_node);
    REGISTER("List: Remove Value", test_list_remove_value);
    REGISTER("OrderedTable: Iteration", test_ordered_table_iteration);
    REGISTER("OrderedTable: Updates and Deletes", test_ordered_table_updates_and_deletes);

#undef REGISTER
}
