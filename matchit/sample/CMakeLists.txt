set(MATCHIT_SAMPLES
        matchit_sample_at_bindings
        matchit_sample_check_and_log_large
        matchit_sample_clip
        matchit_sample_closed_class_hierarchy
        matchit_sample_conditional_if_let_exprs
        matchit_sample_contains
        matchit_sample_custom_as_pointer
        matchit_sample_custom_ds
        matchit_sample_dereference_pattern
        matchit_sample_detect_tuple
        matchit_sample_ds_enums
        matchit_sample_ds_nested_structs_and_enums
        matchit_sample_eval
        matchit_sample_eval_expr_trees
        matchit_sample_extractor_pattern
        matchit_sample_extra_conditionals
        matchit_sample_factorial
        matchit_sample_gcd
        matchit_sample_get_class_name
        matchit_sample_graph
        matchit_sample_identifier_pattern
        matchit_sample_ignoring_parts_of_a_value
        matchit_sample_is_large
        matchit_sample_is_valid
        matchit_sample_literal_pattern
        matchit_sample_matching_variants
        matchit_sample_matching_integrals
        matchit_sample_matching_polymorphic_types
        matchit_sample_matching_strings
        matchit_sample_matching_tuples
        matchit_sample_matching_within
        matchit_sample_optional_lift
        matchit_sample_predicate_based_discriminator
        matchit_sample_quotient_remainder
        matchit_sample_range_pattern
        matchit_sample_rbtree_rebalancing
        matchit_sample_reference_pattern
        matchit_sample_relu
        matchit_sample_rest_pattern
        matchit_sample_slice_pattern
        matchit_sample_some_none
        matchit_sample_structured_binding_pattern
        matchit_sample_struct_pattern
        matchit_sample_sum_is
        matchit_sample_symmetric
        matchit_sample_terminate
        matchit_sample_tuple_struct_pattern
        matchit_sample_variant_any
        matchit_sample_visit
        matchit_sample_wildcard_pattern
        matchit_sample_while_conditional_let_loops)

foreach (sample ${MATCHIT_SAMPLES})
    add_executable(${sample} ${sample}.cpp)
    target_compile_options(${sample} PRIVATE ${BASE_COMPILE_FLAGS})
    set_target_properties(${sample} PROPERTIES CXX_EXTENSIONS OFF)
    add_test(${sample} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${sample})
endforeach ()