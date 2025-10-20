////////////////////////////////////////////////////////////////////////////////
// container_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

void print_list(List* list);

TestGroup container_tests;

static int test_list_remove_node()
{
    // Test 0-initialization as per Object/Mobile Entities
    List list = { 0 };

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

    return 0;
}

static int test_list_remove_value()
{
    // Test 0-initialization as per Object/Mobile Entities
    List list = { 0 };

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

    return 0;
}

void register_container_tests()
{
#define REGISTER(n, f)  register_test(&container_tests, (n), (f))

    init_test_group(&container_tests, "CONTAINER TESTS");
    register_test_group(&container_tests);

    REGISTER("List: Remove Node", test_list_remove_node);
    REGISTER("List: Remove Value", test_list_remove_value);

#undef REGISTER
}