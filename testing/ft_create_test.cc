/*
 * Copyright (c) 2025, valkey-search contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD 3-Clause
 *
 */

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/commands/commands.h"
#include "src/commands/ft_create_parser.h"
#include "src/indexes/index_base.h"
#include "src/schema_manager.h"
#include "testing/common.h"
#include "vmsdk/src/module.h"
#include "vmsdk/src/testing_infra/module.h"
#include "vmsdk/src/valkey_module_api/valkey_module.h"

namespace valkey_search {

namespace {

using ::testing::TestParamInfo;
using ::testing::ValuesIn;

// Helper function to execute FT.CREATE command and handle cleanup
int ExecuteFTCreateCommand(ValkeyModuleCtx* ctx,
                           const std::vector<std::string>& argv,
                           int expected_return = VALKEYMODULE_OK,
                           const std::string& expected_reply = "+OK\r\n",
                           bool clear_reply = true) {
  std::vector<ValkeyModuleString*> cmd_argv;
  std::transform(argv.begin(), argv.end(), std::back_inserter(cmd_argv),
                 [&](std::string val) {
                   return TestValkeyModule_CreateStringPrintf(ctx, "%s",
                                                              val.data());
                 });

  int result =
      vmsdk::CreateCommand<FTCreateCmd>(ctx, cmd_argv.data(), cmd_argv.size());
  EXPECT_EQ(result, expected_return);

  if (!expected_reply.empty()) {
    EXPECT_EQ(ctx->reply_capture.GetReply(), expected_reply);
  }

  if (clear_reply) {
    ctx->reply_capture.ClearReply();
  }

  for (auto cmd_arg : cmd_argv) {
    TestValkeyModule_FreeString(ctx, cmd_arg);
  }

  return result;
}

struct ExpectedIndex {
  std::string attribute_alias;
  indexes::IndexerType indexer_type;
};

struct FTCreateTestCase {
  std::string test_name;
  std::vector<std::string> argv;
  std::string index_schema_name;
  int expected_run_return;
  std::string expected_reply_message;
  std::vector<ExpectedIndex> expected_indexes;
};

class FTCreateTest : public ValkeySearchTestWithParam<FTCreateTestCase> {};

TEST_P(FTCreateTest, FTCreateTests) {
  const FTCreateTestCase& test_case = GetParam();
  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  // Execute the command with the test case parameters
  ExecuteFTCreateCommand(&fake_ctx_, test_case.argv,
                         test_case.expected_run_return,
                         test_case.expected_reply_message);

  auto index_schema = SchemaManager::Instance().GetIndexSchema(
      db_num, test_case.index_schema_name);
  VMSDK_EXPECT_OK(index_schema);
  for (const auto& expected_index : test_case.expected_indexes) {
    auto index = index_schema.value()->GetIndex(expected_index.attribute_alias);
    VMSDK_EXPECT_OK(index);
    EXPECT_EQ(index.value()->GetIndexerType(), expected_index.indexer_type);
  }
  VMSDK_EXPECT_OK(SchemaManager::Instance().RemoveIndexSchema(
      db_num, test_case.index_schema_name));
}

INSTANTIATE_TEST_SUITE_P(
    FTCreateTests, FTCreateTest,
    ValuesIn<FTCreateTestCase>({
        {
            .test_name = "happy_path_hnsw",
            .argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                     "vector", "HNSW", "12", "m", "100", "TYPE", "FLOAT32",
                     "DIM", "100", "DISTANCE_METRIC", "IP", "EF_CONSTRUCTION",
                     "40", "INITIAL_CAP", "15000"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "vector",
                        .indexer_type = indexes::IndexerType::kHNSW,
                    },
                },
        },
        {
            .test_name = "happy_path_hnsw_with_numeric",
            .argv = {"FT.CREATE", "test_index_schema",
                     "schema",    "field1",
                     "numeric",   "vector",
                     "vector",    "HNSW",
                     "12",        "m",
                     "100",       "TYPE",
                     "FLOAT32",   "DIM",
                     "100",       "DISTANCE_METRIC",
                     "IP",        "EF_CONSTRUCTION",
                     "40",        "INITIAL_CAP",
                     "15000"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "field1",
                        .indexer_type = indexes::IndexerType::kNumeric,
                    },
                    {
                        .attribute_alias = "vector",
                        .indexer_type = indexes::IndexerType::kHNSW,
                    },
                },
        },
        {
            .test_name = "happy_path_flat",
            .argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                     "vector", "Flat", "8", "TYPE", "FLOAT32", "DIM", "100",
                     "DISTANCE_METRIC", "IP", "INITIAL_CAP", "15000"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "vector",
                        .indexer_type = indexes::IndexerType::kFlat,
                    },
                },
        },
        {
            .test_name = "happy_path_text_with_options",
            .argv = {"FT.CREATE", "test_index_schema", "SCHEMA", "description",
                     "text", "NOSTEM", "MINSTEMSIZE", "5"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "description",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "happy_path_text_with_vector",
            .argv = {"FT.CREATE", "test_index_schema", "SCHEMA", "content",
                     "text", "WITHSUFFIXTRIE", "vector", "vector", "HNSW", "14",
                     "TYPE", "FLOAT32", "DIM", "128", "DISTANCE_METRIC", "L2",
                     "M", "16", "EF_CONSTRUCTION", "200", "INITIAL_CAP", "1000",
                     "EF_RUNTIME", "100"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "content",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                    {
                        .attribute_alias = "vector",
                        .indexer_type = indexes::IndexerType::kHNSW,
                    },
                },
        },
        {
            .test_name = "happy_path_multiple_text_fields",
            .argv = {"FT.CREATE", "test_index_schema", "SCHEMA", "title", "text",
                     "NOSTEM", "description", "text", "MINSTEMSIZE", "4",
                     "content", "text", "WITHSUFFIXTRIE"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                    {
                        .attribute_alias = "description",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                    {
                        .attribute_alias = "content",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "happy_path_flat_with_tag",
            .argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                     "vector", "Flat", "8", "TYPE", "FLOAT32", "DIM", "100",
                     "DISTANCE_METRIC", "IP", "INITIAL_CAP", "15000", "field1",
                     "tag", "separator", "|"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "field1",
                        .indexer_type = indexes::IndexerType::kTag,
                    },
                    {
                        .attribute_alias = "vector",
                        .indexer_type = indexes::IndexerType::kFlat,
                    },
                },
        },
        {
            .test_name = "happy_path_text",
            .argv = {"FT.CREATE", "test_index_schema", "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "happy_path_text_with_tag_and_numeric",
            .argv = {"FT.CREATE", "test_index_schema", "SCHEMA", "title", "text",
                     "MINSTEMSIZE", "2", "tags", "tag", "SEPARATOR", ",", "score",
                     "numeric"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                    {
                        .attribute_alias = "tags",
                        .indexer_type = indexes::IndexerType::kTag,
                    },
                    {
                        .attribute_alias = "score",
                        .indexer_type = indexes::IndexerType::kNumeric,
                    },
                },
        },
        // Schema-level text processing options tests
        {
            .test_name = "schema_punctuation_setting",
            .argv = {"FT.CREATE", "test_index_schema", "PUNCTUATION", ".,!?;:", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_nooffsets_setting",
            .argv = {"FT.CREATE", "test_index_schema", "NOOFFSETS", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_withoffsets_setting",
            .argv = {"FT.CREATE", "test_index_schema", "WITHOFFSETS", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_stopwords_setting",
            .argv = {"FT.CREATE", "test_index_schema", "STOPWORDS", "3", "foo", "bar", "baz", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_nostopwords_setting",
            .argv = {"FT.CREATE", "test_index_schema", "NOSTOPWORDS", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_language_setting",
            .argv = {"FT.CREATE", "test_index_schema", "LANGUAGE", "english", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_nostem_setting",
            .argv = {"FT.CREATE", "test_index_schema", "NOSTEM", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_minstemsize_setting",
            .argv = {"FT.CREATE", "test_index_schema", "MINSTEMSIZE", "3", 
                     "SCHEMA", "title", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_combined_text_settings",
            .argv = {"FT.CREATE", "test_index_schema", "PUNCTUATION", ".,!?", 
                     "NOOFFSETS", "STOPWORDS", "2", "the", "and", "NOSTEM",
                     "SCHEMA", "title", "text", "content", "text"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                    {
                        .attribute_alias = "content",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                },
        },
        {
            .test_name = "schema_text_with_mixed_field_types",
            .argv = {"FT.CREATE", "test_index_schema", "PUNCTUATION", ".,!?", 
                     "WITHOFFSETS", "LANGUAGE", "english", "MINSTEMSIZE", "5",
                     "SCHEMA", "title", "text", "tags", "tag", "score", "numeric",
                     "vector", "vector", "HNSW", "6", "TYPE", "FLOAT32", "DIM", "3",
                     "DISTANCE_METRIC", "IP"},
            .index_schema_name = "test_index_schema",
            .expected_run_return = VALKEYMODULE_OK,
            .expected_reply_message = "+OK\r\n",
            .expected_indexes =
                {
                    {
                        .attribute_alias = "title",
                        .indexer_type = indexes::IndexerType::kText,
                    },
                    {
                        .attribute_alias = "tags",
                        .indexer_type = indexes::IndexerType::kTag,
                    },
                    {
                        .attribute_alias = "score",
                        .indexer_type = indexes::IndexerType::kNumeric,
                    },
                    {
                        .attribute_alias = "vector",
                        .indexer_type = indexes::IndexerType::kHNSW,
                    },
                },
        },
    }),
    [](const TestParamInfo<FTCreateTestCase>& info) {
      return info.param.test_name;
    });

// Test to verify the max-indexes limit
TEST_F(FTCreateTest, MaxIndexesLimit) {
  // Set max-indexes to 2 for this test
  VMSDK_EXPECT_OK(options::GetMaxIndexes().SetValue(2));

  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  std::vector<std::string> argv = {"FT.CREATE", "test_index_schema",
                                   "schema",    "vector",
                                   "vector",    "Flat",
                                   "8",         "TYPE",
                                   "FLOAT32",   "DIM",
                                   "100",       "DISTANCE_METRIC",
                                   "IP",        "INITIAL_CAP",
                                   "15000"};

  // Create 2 indexes successfully
  for (int i = 0; i < 2; i++) {
    // Change index and vector ids
    argv[1] = absl::StrCat(argv[1], i);
    argv[3] = absl::StrCat(argv[3], i);

    // Execute command and expect success
    ExecuteFTCreateCommand(&fake_ctx_, argv);
  }

  // Try to create a third index
  argv[1] = absl::StrCat(argv[1], 2);
  argv[3] = absl::StrCat(argv[3], 2);

  // Execute command with empty expected reply (we'll check it separately)
  ExecuteFTCreateCommand(
      &fake_ctx_, argv, VALKEYMODULE_OK,
      "$108\r\nInvalid range: Value above maximum; Maximum number of indexes "
      "reached (2). Cannot create additional indexes.\r\n");
}

// Struct to hold parameters for max limit tests
struct MaxLimitTestCase {
  std::string test_name;
  std::function<absl::Status(long long)> set_limit_func;
  std::function<absl::Status(long long)> reset_limit_func;
  std::vector<std::string> valid_argv;
  std::vector<std::string> exceed_argv;
  std::string expected_error_message;
};

class MaxLimitTest : public ValkeySearchTestWithParam<MaxLimitTestCase> {};

TEST_P(MaxLimitTest, MaxLimitTests) {
  const MaxLimitTestCase& test_case = GetParam();

  // Set the limit to a small value for this test
  VMSDK_EXPECT_OK(test_case.set_limit_func(5));

  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  // Create an index with valid parameters (within limits)
  ExecuteFTCreateCommand(&fake_ctx_, test_case.valid_argv);

  // Try to create an index that exceeds the limit
  ExecuteFTCreateCommand(&fake_ctx_, test_case.exceed_argv, VALKEYMODULE_OK,
                         test_case.expected_error_message);
}

INSTANTIATE_TEST_SUITE_P(
    MaxLimitTests, MaxLimitTest,
    ValuesIn<MaxLimitTestCase>({
        {
            .test_name = "MaxPrefixesLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxPrefixes().SetValue(2);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "PREFIX", "2",
                           "prefix1", "prefix2", "schema", "vector", "vector",
                           "Flat", "8", "TYPE", "FLOAT32", "DIM", "100",
                           "DISTANCE_METRIC", "IP", "INITIAL_CAP", "15000"},
            .exceed_argv = {"FT.CREATE",
                            "test_index_schema2",
                            "PREFIX",
                            "3",
                            "prefix1",
                            "prefix2",
                            "prefix3",
                            "schema",
                            "vector",
                            "vector",
                            "Flat",
                            "8",
                            "TYPE",
                            "FLOAT32",
                            "DIM",
                            "100",
                            "DISTANCE_METRIC",
                            "IP",
                            "INITIAL_CAP",
                            "15000"},
            .expected_error_message =
                "$90\r\nInvalid range: Value above maximum; Number of prefixes "
                "(3) exceeds the maximum allowed (2)\r\n",
        },
        {
            .test_name = "MaxTagFieldLengthLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxTagFieldLen().SetValue(5);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "Flat", "8", "TYPE", "FLOAT32", "DIM",
                           "100", "DISTANCE_METRIC", "IP", "INITIAL_CAP",
                           "15000", "field", "tag", "separator", "|"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2", "schema",
                            "vector", "vector", "Flat", "8", "TYPE", "FLOAT32",
                            "DIM", "100", "DISTANCE_METRIC", "IP",
                            "INITIAL_CAP", "15000", "field_too_long", "tag",
                            "separator", "|"},
            .expected_error_message =
                "$126\r\nInvalid field type for field `field_too_long`: "
                "Invalid range: Value above maximum; A tag field can have a "
                "maximum length of 5.\r\n",
        },
        {
            .test_name = "MaxNumericFieldLengthLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxNumericFieldLen().SetValue(5);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "Flat", "8", "TYPE", "FLOAT32", "DIM",
                           "100", "DISTANCE_METRIC", "IP", "INITIAL_CAP",
                           "15000", "field", "numeric"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2", "schema",
                            "vector", "vector", "Flat", "8", "TYPE", "FLOAT32",
                            "DIM", "100", "DISTANCE_METRIC", "IP",
                            "INITIAL_CAP", "15000", "field_too_long",
                            "numeric"},
            .expected_error_message =
                "$130\r\nInvalid field type for field `field_too_long`: "
                "Invalid range: Value above maximum; A numeric field can have "
                "a maximum length of 5.\r\n",
        },
        {
            .test_name = "MaxAttributesLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxAttributes().SetValue(1);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "HNSW", "6", "TYPE", "FLOAT32", "DIM", "3",
                           "DISTANCE_METRIC", "IP"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2",
                            "schema",    "vector1",
                            "vector",    "HNSW",
                            "6",         "TYPE",
                            "FLOAT32",   "DIM",
                            "3",         "DISTANCE_METRIC",
                            "IP",        "vector2",
                            "vector",    "HNSW",
                            "6",         "TYPE",
                            "FLOAT32",   "DIM",
                            "3",         "DISTANCE_METRIC",
                            "IP"},
            .expected_error_message =
                "$85\r\nInvalid range: Value above maximum; The maximum number "
                "of attributes cannot exceed 1.\r\n",
        },
        {
            .test_name = "MaxDimensionsLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxDimensions().SetValue(10);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "HNSW", "6", "TYPE", "FLOAT32", "DIM",
                           "10", "DISTANCE_METRIC", "IP"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2", "schema",
                            "vector", "vector", "HNSW", "6", "TYPE", "FLOAT32",
                            "DIM", "11", "DISTANCE_METRIC", "IP"},
            .expected_error_message =
                "$167\r\nInvalid field type for field `vector`: Invalid range: "
                "Value above maximum; The dimensions value must be a positive "
                "integer greater than 0 and less than or equal to 10.\r\n",
        },
        {
            .test_name = "MaxMLimit",
            .set_limit_func =
                [](long long value) { return options::GetMaxM().SetValue(50); },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "HNSW", "8", "TYPE", "FLOAT32", "DIM", "3",
                           "DISTANCE_METRIC", "IP", "M", "50"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2", "schema",
                            "vector", "vector", "HNSW", "8", "TYPE", "FLOAT32",
                            "DIM", "3", "DISTANCE_METRIC", "IP", "M", "51"},
            .expected_error_message =
                "$140\r\nInvalid field type for field `vector`: Invalid range: "
                "Value above maximum; M must be a positive integer greater "
                "than 0 and cannot exceed 50.\r\n",
        },
        {
            .test_name = "MaxEfConstructionLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxEfConstruction().SetValue(200);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "HNSW", "8", "TYPE", "FLOAT32", "DIM", "3",
                           "DISTANCE_METRIC", "IP", "EF_CONSTRUCTION", "200"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2", "schema",
                            "vector", "vector", "HNSW", "8", "TYPE", "FLOAT32",
                            "DIM", "3", "DISTANCE_METRIC", "IP",
                            "EF_CONSTRUCTION", "201"},
            .expected_error_message =
                "$155\r\nInvalid field type for field `vector`: Invalid range: "
                "Value above maximum; EF_CONSTRUCTION must be a positive "
                "integer greater than 0 and cannot exceed 200.\r\n",
        },
        {
            .test_name = "MaxEfRuntimeLimit",
            .set_limit_func =
                [](long long value) {
                  return options::GetMaxEfRuntime().SetValue(100);
                },
            .valid_argv = {"FT.CREATE", "test_index_schema", "schema", "vector",
                           "vector", "HNSW", "8", "TYPE", "FLOAT32", "DIM", "3",
                           "DISTANCE_METRIC", "IP", "EF_RUNTIME", "100"},
            .exceed_argv = {"FT.CREATE", "test_index_schema2", "schema",
                            "vector", "vector", "HNSW", "8", "TYPE", "FLOAT32",
                            "DIM", "3", "DISTANCE_METRIC", "IP", "EF_RUNTIME",
                            "101"},
            .expected_error_message =
                "$150\r\nInvalid field type for field `vector`: Invalid range: "
                "Value above maximum; EF_RUNTIME must be a positive integer "
                "greater than 0 and cannot exceed 100.\r\n",
        },
    }),
    [](const TestParamInfo<MaxLimitTestCase>& info) {
      return info.param.test_name;
    });

// Test to verify schema settings are properly preserved
TEST_F(FTCreateTest, SchemaSettingsPreservation) {
  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  // Create an index with specific schema settings (avoid conflicting NOSTEM + LANGUAGE)
  std::vector<std::string> argv = {
      "FT.CREATE", "test_schema_settings",
      "PUNCTUATION", ".,!?;:",
      "NOOFFSETS",
      "STOPWORDS", "2", "foo", "bar", 
      "MINSTEMSIZE", "7",
      "SCHEMA", "title", "text"
  };
  
  ExecuteFTCreateCommand(&fake_ctx_, argv);

  // Get the created index schema
  auto index_schema_result = SchemaManager::Instance().GetIndexSchema(db_num, "test_schema_settings");
  VMSDK_EXPECT_OK(index_schema_result);
  auto index_schema = index_schema_result.value();

  // Verify all schema settings are preserved
  EXPECT_EQ(index_schema->GetPunctuation(), ".,!?;:");
  EXPECT_FALSE(index_schema->GetWithOffsets());  // NOOFFSETS was set
  
  auto stop_words = index_schema->GetStopWords();
  EXPECT_EQ(stop_words.size(), 2);
  EXPECT_TRUE(std::find(stop_words.begin(), stop_words.end(), "foo") != stop_words.end());
  EXPECT_TRUE(std::find(stop_words.begin(), stop_words.end(), "bar") != stop_words.end());
  
  EXPECT_EQ(index_schema->GetTextLanguage(), data_model::LANGUAGE_ENGLISH);  // Default language
  EXPECT_FALSE(index_schema->GetNostem());  // Default is to use stemming
  EXPECT_EQ(index_schema->GetMinStemSize(), 7);

  // Verify ToProto() preserves all settings
  auto proto = index_schema->ToProto();
  EXPECT_EQ(proto->punctuation(), ".,!?;:");
  EXPECT_FALSE(proto->with_offsets());
  EXPECT_EQ(proto->stop_words_size(), 2);
  EXPECT_EQ(proto->stop_words(0), "foo");
  EXPECT_EQ(proto->stop_words(1), "bar");
  EXPECT_EQ(proto->language(), data_model::LANGUAGE_ENGLISH);
  EXPECT_FALSE(proto->nostem());  // Default is to use stemming
  EXPECT_EQ(proto->min_stem_size(), 7);

  VMSDK_EXPECT_OK(SchemaManager::Instance().RemoveIndexSchema(db_num, "test_schema_settings"));
}

// Test to verify NOSTEM setting separately
TEST_F(FTCreateTest, SchemaNoStemSetting) {
  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  // Create an index with NOSTEM (without LANGUAGE to avoid conflict)
  std::vector<std::string> argv = {
      "FT.CREATE", "test_nostem",
      "NOSTEM",
      "SCHEMA", "title", "text"
  };
  
  ExecuteFTCreateCommand(&fake_ctx_, argv);

  // Get the created index schema
  auto index_schema_result = SchemaManager::Instance().GetIndexSchema(db_num, "test_nostem");
  VMSDK_EXPECT_OK(index_schema_result);
  auto index_schema = index_schema_result.value();

  // Verify NOSTEM setting is preserved
  EXPECT_TRUE(index_schema->GetNostem());

  // Verify ToProto() preserves NOSTEM setting
  auto proto = index_schema->ToProto();
  EXPECT_TRUE(proto->nostem());

  VMSDK_EXPECT_OK(SchemaManager::Instance().RemoveIndexSchema(db_num, "test_nostem"));
}

// Test to verify LANGUAGE setting separately
TEST_F(FTCreateTest, SchemaLanguageSetting) {
  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  // Create an index with LANGUAGE (without NOSTEM to avoid conflict)
  std::vector<std::string> argv = {
      "FT.CREATE", "test_language",
      "LANGUAGE", "english",
      "SCHEMA", "title", "text"
  };
  
  ExecuteFTCreateCommand(&fake_ctx_, argv);

  // Get the created index schema
  auto index_schema_result = SchemaManager::Instance().GetIndexSchema(db_num, "test_language");
  VMSDK_EXPECT_OK(index_schema_result);
  auto index_schema = index_schema_result.value();

  // Verify LANGUAGE setting is preserved
  EXPECT_EQ(index_schema->GetTextLanguage(), data_model::LANGUAGE_ENGLISH);
  EXPECT_FALSE(index_schema->GetNostem());  // Should be false when language is specified

  // Verify ToProto() preserves LANGUAGE setting
  auto proto = index_schema->ToProto();
  EXPECT_EQ(proto->language(), data_model::LANGUAGE_ENGLISH);
  EXPECT_FALSE(proto->nostem());

  VMSDK_EXPECT_OK(SchemaManager::Instance().RemoveIndexSchema(db_num, "test_language"));
}

// Test to verify default schema settings
TEST_F(FTCreateTest, DefaultSchemaSettings) {
  int db_num = 1;
  ON_CALL(*kMockValkeyModule, GetSelectedDb(&fake_ctx_))
      .WillByDefault(testing::Return(db_num));

  // Create an index without explicit schema settings
  std::vector<std::string> argv = {
      "FT.CREATE", "test_defaults",
      "SCHEMA", "title", "text"
  };
  
  ExecuteFTCreateCommand(&fake_ctx_, argv);

  // Get the created index schema
  auto index_schema_result = SchemaManager::Instance().GetIndexSchema(db_num, "test_defaults");
  VMSDK_EXPECT_OK(index_schema_result);
  auto index_schema = index_schema_result.value();

  // Verify default settings are applied
  EXPECT_TRUE(index_schema->GetWithOffsets());  // Default is WITHOFFSETS
  EXPECT_EQ(index_schema->GetTextLanguage(), data_model::LANGUAGE_ENGLISH);  // Default language
  EXPECT_FALSE(index_schema->GetNostem());  // Default is to use stemming
  EXPECT_EQ(index_schema->GetMinStemSize(), 4);  // Default min stem size

  VMSDK_EXPECT_OK(SchemaManager::Instance().RemoveIndexSchema(db_num, "test_defaults"));
}

}  // namespace

}  // namespace valkey_search
