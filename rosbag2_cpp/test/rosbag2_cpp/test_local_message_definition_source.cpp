// Copyright 2022, Foxglove Technologies. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <set>
#include <string>

#include "gmock/gmock.h"
#include "rosbag2_cpp/message_definitions/local_message_definition_source.hpp"

using rosbag2_cpp::LocalMessageDefinitionSource;
using rosbag2_cpp::parse_definition_dependencies;
using ::testing::UnorderedElementsAre;

TEST(test_local_message_definition_source, can_find_idl_includes)
{
  const char sample[] =
    R"r(
#include "rosbag2_test_msgdefs/msg/BasicIdlA.idl"

#include <rosbag2_test_msgdefs/msg/BasicIdlB.idl>

module rosbag2_test_msgdefs {
  module msg {
    struct ComplexIdl {
      rosbag2_test_msgdefs::msg::BasicIdlA a;
      rosbag2_test_msgdefs::msg::BasicIdlB b;
    };
  };
};

  )r";
  std::set<std::string> dependencies = parse_definition_dependencies(
    LocalMessageDefinitionSource::Format::IDL, sample, "", "");
  EXPECT_THAT(
    dependencies, UnorderedElementsAre(
      "rosbag2_test_msgdefs/msg/BasicIdlA",
      "rosbag2_test_msgdefs/msg/BasicIdlB"));
}

TEST(test_local_message_definition_source, can_find_relative_idl_includes)
{
  const char sample[] =
    R"r(
#include "BasicIdlA.idl"

#include <rosbag2_test_msgdefs/msg/BasicIdlB.idl>

module rosbag2_test_msgdefs {
  module msg {
    struct ComplexIdl {
      rosbag2_test_msgdefs::msg::BasicIdlA a;
      rosbag2_test_msgdefs::msg::BasicIdlB b;
    };
  };
};

  )r";
  std::set<std::string> dependencies = parse_definition_dependencies(
    LocalMessageDefinitionSource::Format::IDL, sample, "rosbag2_test_msgdefs", "msg");
  EXPECT_THAT(
    dependencies, UnorderedElementsAre(
    "rosbag2_test_msgdefs/msg/BasicIdlA",
    "rosbag2_test_msgdefs/msg/BasicIdlB"));
}

TEST(test_local_message_definition_source, can_find_msg_deps)
{
  LocalMessageDefinitionSource source;
  auto result = source.get_full_text_ext("rosbag2_test_msgdefs/ComplexMsg", "/complex_msg_topic");
  EXPECT_EQ(result.encoding, "ros2msg");
  EXPECT_EQ(
    result.encoded_message_definition,
    "rosbag2_test_msgdefs/BasicMsg b\n"
    "\n"
    "================================================================================\n"
    "MSG: rosbag2_test_msgdefs/BasicMsg\n"
    "float32 c\n");
}

TEST(test_local_message_definition_source, can_find_msg_definition_in_nested_subfolder)
{
  LocalMessageDefinitionSource source;
  auto result =
    source.get_full_text_ext("rosbag2_test_msgdefs/msg/nested_sub_dir/AnotherBasicMsg",
                             "/basic_msg_topic");
  EXPECT_EQ(result.encoding, "ros2msg");
  // Note: By design The top-level message definition for MSG format is present first, with no
  // delimiter. All dependent .msg definitions are preceded by a two-line delimiter:
  EXPECT_EQ(result.encoded_message_definition, "float32 c\n");
}

TEST(test_local_message_definition_source, can_find_action_definition_in_nested_subfolder)
{
  LocalMessageDefinitionSource source;
  auto result =
    source.get_full_text_ext("rosbag2_test_msgdefs/nested_sub_dir/action/BasicMsg",
                             "/basic_action_msg/_action/send_goal");
  EXPECT_EQ(result.encoding, "ros2msg");
  EXPECT_EQ(result.encoded_message_definition,
    "================================================================================\n"
    "ACTION: rosbag2_test_msgdefs/nested_sub_dir/action/BasicMsg\n"
    "string goal\n"
    "---\n"
    "string result\n"
    "---\n"
    "string feedback\n"
  );
}

TEST(test_local_message_definition_source, can_find_srv_deps_in_msg)
{
  LocalMessageDefinitionSource source;
  auto result = source.get_full_text_ext("rosbag2_test_msgdefs/srv/ComplexSrvMsg_Event",
    "/complex_srv_msg_topic/_service_event");
  EXPECT_EQ(result.encoding, "ros2msg");
  EXPECT_EQ(
    result.encoded_message_definition,
    "service_msgs/msg/ServiceEventInfo info\n"
    "rosbag2_test_msgdefs/srv/ComplexSrvMsg_Request[<=1] request\n"
    "rosbag2_test_msgdefs/srv/ComplexSrvMsg_Response[<=1] response\n"
    "\n"
    "================================================================================\n"
    "MSG: rosbag2_test_msgdefs/srv/ComplexSrvMsg_Request\n"
    "rosbag2_test_msgdefs/BasicMsg req\n"
    "\n"
    "================================================================================\n"
    "MSG: rosbag2_test_msgdefs/BasicMsg\n"
    "float32 c\n"
    "\n"
    "================================================================================\n"
    "MSG: rosbag2_test_msgdefs/srv/ComplexSrvMsg_Response\n"
    "rosbag2_test_msgdefs/BasicMsg resp\n"
    "\n"
    "================================================================================\n"
    "MSG: service_msgs/msg/ServiceEventInfo\n"
    "uint8 REQUEST_SENT = 0\n"
    "uint8 REQUEST_RECEIVED = 1\n"
    "uint8 RESPONSE_SENT = 2\n"
    "uint8 RESPONSE_RECEIVED = 3\n"
    "\n"
    "# The type of event this message represents\n"
    "uint8 event_type\n"
    "\n"
    "# Timestamp for when the event occurred (sent or received time)\n"
    "builtin_interfaces/Time stamp\n"
    "\n"
    "# Unique identifier for the client that sent the service request\n"
    "# Note, this is only unique for the current session.\n"
    "# The size here has to match the size of rmw_dds_common/msg/Gid,\n"
    "# but unfortunately we cannot use that message directly due to a\n"
    "# circular dependency.\n"
    "char[16] client_gid\n"
    "\n"
    "# Sequence number for the request\n"
    "# Combined with the client ID, this creates a unique ID for the service transaction\n"
    "int64 sequence_number\n"
    "\n"
    "================================================================================\n"
    "MSG: builtin_interfaces/Time\n"
    "# This message communicates ROS Time defined here:\n"
    "# https://design.ros2.org/articles/clock_and_time.html\n"
    "\n"
    "# The seconds component, valid over all int32 values.\n"
    "int32 sec\n"
    "\n"
    "# The nanoseconds component, valid in the range [0, 1e9), to be added to the seconds component. \n"
    "# e.g.\n"
    "# The time -1.7 seconds is represented as {sec: -2, nanosec: 3e8}\n"
    "# The time 1.7 seconds is represented as {sec: 1, nanosec: 7e8}\n"
    "uint32 nanosec\n") << result.encoded_message_definition << std::endl;
}

TEST(test_local_message_definition_source, can_find_srv_deps_in_idl)
{
  LocalMessageDefinitionSource source;
  auto result = source.get_full_text_ext("rosbag2_test_msgdefs/srv/ComplexSrvIdl_Event",
    "/complex_srv_idl_topic/_service_event");
  EXPECT_EQ(result.encoding, "ros2idl");
  EXPECT_EQ(
    result.encoded_message_definition,
    "service_msgs/msg/ServiceEventInfo info\n"
    "rosbag2_test_msgdefs/srv/ComplexSrvIdl_Request[<=1] request\n"
    "rosbag2_test_msgdefs/srv/ComplexSrvIdl_Response[<=1] response\n"
    "\n"
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/srv/ComplexSrvIdl_Request\n"
    "rosbag2_test_msgdefs/BasicIdl req\n"
    "\n"
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/BasicIdl\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct BasicIdl {\n"
    "        float x;\n"
    "    };\n"
    "  };\n"
    "};\n"
    "\n"
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/srv/ComplexSrvIdl_Response\n"
    "rosbag2_test_msgdefs/BasicIdl resp\n"
    "\n"
    "================================================================================\n"
    "MSG: service_msgs/msg/ServiceEventInfo\n"
    "uint8 REQUEST_SENT = 0\n"
    "uint8 REQUEST_RECEIVED = 1\n"
    "uint8 RESPONSE_SENT = 2\n"
    "uint8 RESPONSE_RECEIVED = 3\n"
    "\n"
    "# The type of event this message represents\n"
    "uint8 event_type\n"
    "\n"
    "# Timestamp for when the event occurred (sent or received time)\n"
    "builtin_interfaces/Time stamp\n"
    "\n"
    "# Unique identifier for the client that sent the service request\n"
    "# Note, this is only unique for the current session.\n"
    "# The size here has to match the size of rmw_dds_common/msg/Gid,\n"
    "# but unfortunately we cannot use that message directly due to a\n"
    "# circular dependency.\n"
    "char[16] client_gid\n"
    "\n"
    "# Sequence number for the request\n"
    "# Combined with the client ID, this creates a unique ID for the service transaction\n"
    "int64 sequence_number\n"
    "\n"
    "================================================================================\n"
    "MSG: builtin_interfaces/Time\n"
    "# This message communicates ROS Time defined here:\n"
    "# https://design.ros2.org/articles/clock_and_time.html\n"
    "\n"
    "# The seconds component, valid over all int32 values.\n"
    "int32 sec\n"
    "\n"
    "# The nanoseconds component, valid in the range [0, 1e9), to be added to the seconds component. \n"
    "# e.g.\n"
    "# The time -1.7 seconds is represented as {sec: -2, nanosec: 3e8}\n"
    "# The time 1.7 seconds is represented as {sec: 1, nanosec: 7e8}\n"
    "uint32 nanosec\n") << result.encoded_message_definition << std::endl;
}

TEST(test_local_message_definition_source, can_find_action_deps_in_msg)
{
  auto check_result = [](const rosbag2_storage::MessageDefinition & result) {
      EXPECT_EQ(result.encoding, "ros2msg");
      EXPECT_EQ(
        result.encoded_message_definition,
        "================================================================================\n"
        "ACTION: rosbag2_test_msgdefs/action/ComplexActionMsg\n"
        "rosbag2_test_msgdefs/BasicMsg goal\n"
        "---\n"
        "rosbag2_test_msgdefs/BasicMsg result\n"
        "---\n"
        "rosbag2_test_msgdefs/BasicMsg feedback\n"
        "\n"
        "================================================================================\n"
        "MSG: rosbag2_test_msgdefs/BasicMsg\n"
        "float32 c\n") << result.encoded_message_definition << std::endl;
    };

  LocalMessageDefinitionSource source;

  // Check action interface send_goal
  {
    auto result = source.get_full_text_ext(
      "rosbag2_test_msgdefs/action/ComplexActionMsg_SendGoal_Event",
      "/complex_action_msg/_action/send_goal/_service_event");
    check_result(result);
  }

  // Known limitation:
  // The get_full_text_ext(..) can return action definition for action interface 'cancel_goal' and
  // 'status' only if it was a prior successful call to the get_full_text_ext(..) for the same
  // action name.
  // Check action interface cancel_goal
  {
    auto result = source.get_full_text_ext(
      "action_msgs/srv/CancelGoal_Event",
      "/complex_action_msg/_action/cancel_goal/_service_event");
    check_result(result);
  }

  // Check action interface status
  {
    auto result = source.get_full_text_ext(
      "action_msgs/msg/GoalStatusArray",
      "/complex_action_msg/_action/status");
    check_result(result);
  }

  // Check action interface get_result
  {
    auto result = source.get_full_text_ext(
      "rosbag2_test_msgdefs/action/ComplexActionMsg_GetResult_Event",
      "/complex_action_msg/_action/get_result/_service_event");
    check_result(result);
  }

  // Check action interface feedback
  {
    auto result = source.get_full_text_ext(
      "rosbag2_test_msgdefs/action/ComplexActionMsg_FeedbackMessage",
      "/complex_action_msg/_action/feedback");
    check_result(result);
  }
}

TEST(test_local_message_definition_source, can_find_action_deps_in_idl)
{
  auto check_result = [](const rosbag2_storage::MessageDefinition & result) {
      EXPECT_EQ(result.encoding, "ros2idl");
      EXPECT_EQ(
        result.encoded_message_definition,
        "================================================================================\n"
        "ACTION: rosbag2_test_msgdefs/action/ComplexActionIdl\n"
        "rosbag2_test_msgdefs/BasicIdl goal\n"
        "---\n"
        "rosbag2_test_msgdefs/BasicIdl result\n"
        "---\n"
        "rosbag2_test_msgdefs/BasicIdl feedback\n"
        "\n"
        "================================================================================\n"
        "MSG: rosbag2_test_msgdefs/BasicIdl\n"
        "\n"
        "================================================================================\n"
        "IDL: rosbag2_test_msgdefs/BasicIdl\n"
        "module rosbag2_test_msgdefs {\n"
        "  module msg {\n"
        "    struct BasicIdl {\n"
        "        float x;\n"
        "    };\n"
        "  };\n"
        "};\n") << result.encoded_message_definition << std::endl;
    };

  LocalMessageDefinitionSource source;

  // Check action interface send_goal
  {
    auto result = source.get_full_text_ext(
      "rosbag2_test_msgdefs/action/ComplexActionIdl_SendGoal_Event",
      "/complex_action_idl/_action/send_goal/_service_event");
    check_result(result);
  }

  // Known limitation:
  // The get_full_text_ext(..) can return action definition for action interface 'cancel_goal' and
  // 'status' only if it was a prior successful call to the get_full_text_ext(..) for the same
  // action name.
  // Check action interface cancel_goal
  {
    auto result = source.get_full_text_ext(
      "action_msgs/srv/CancelGoal_Event",
      "/complex_action_idl/_action/cancel_goal/_service_event");
    check_result(result);
  }

  // Check action interface status
  {
    auto result = source.get_full_text_ext(
      "action_msgs/msg/GoalStatusArray",
      "/complex_action_idl/_action/status");
    check_result(result);
  }

  // Check action interface get_result
  {
    auto result = source.get_full_text_ext(
      "rosbag2_test_msgdefs/action/ComplexActionIdl_GetResult_Event",
      "/complex_action_idl/_action/get_result/_service_event");
    check_result(result);
  }

  // Check action interface feedback
  {
    auto result = source.get_full_text_ext(
      "rosbag2_test_msgdefs/action/ComplexActionIdl_FeedbackMessage",
      "/complex_action_idl/_action/feedback");
    check_result(result);
  }
}

TEST(test_local_message_definition_source, can_find_idl_deps)
{
  LocalMessageDefinitionSource source;
  auto result = source.get_full_text_ext(
    "rosbag2_test_msgdefs/msg/ComplexIdl", "/complex_idl_topic");
  EXPECT_EQ(result.encoding, "ros2idl");
  EXPECT_EQ(
    result.encoded_message_definition,
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/msg/ComplexIdl\n"
    "#include \"rosbag2_test_msgdefs/msg/BasicIdl.idl\"\n"
    "\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct ComplexIdl {\n"
    "      rosbag2_test_msgdefs::msg::BasicIdl a;\n"
    "    };\n"
    "  };\n"
    "};\n"
    "\n"
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/msg/BasicIdl\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct BasicIdl {\n"
    "        float x;\n"
    "    };\n"
    "  };\n"
    "};\n");
}

TEST(test_local_message_definition_source, can_find_relative_idl_deps)
{
  LocalMessageDefinitionSource source;
  auto result = source.get_full_text_ext(
    "rosbag2_test_msgdefs/msg/ComplexWithRelativeDep", "/complex_idl_topic");
  EXPECT_EQ(result.encoding, "ros2idl");
  EXPECT_EQ(
    result.encoded_message_definition,
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/msg/ComplexWithRelativeDep\n"
    "#include \"BasicIdl.idl\"\n"
    "\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct ComplexWithRelativeDep {\n"
    "      rosbag2_test_msgdefs::msg::BasicIdl a;\n"
    "    };\n"
    "  };\n"
    "};\n"
    "\n"
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/msg/BasicIdl\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct BasicIdl {\n"
    "        float x;\n"
    "    };\n"
    "  };\n"
    "};\n");
}

TEST(test_local_message_definition_source, can_resolve_msg_with_idl_deps)
{
  LocalMessageDefinitionSource source;
  auto result = source.get_full_text_ext(
    "rosbag2_test_msgdefs/msg/ComplexMsgDependsOnIdl", "/complex_msg_depends_on_idl_topic");
  EXPECT_EQ(result.encoding, "ros2idl");
  EXPECT_EQ(
    result.encoded_message_definition,
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/msg/ComplexMsgDependsOnIdl\n"
    "// generated from rosidl_adapter/resource/msg.idl.em\n"
    "// with input from rosbag2_test_msgdefs/msg/ComplexMsgDependsOnIdl.msg\n"
    "// generated code does not contain a copyright notice\n"
    "\n"
    "#include \"rosbag2_test_msgdefs/msg/BasicIdl.idl\"\n"
    "\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct ComplexMsgDependsOnIdl {\n"
    "      rosbag2_test_msgdefs::msg::BasicIdl a;\n"
    "    };\n"
    "  };\n"
    "};\n"
    "\n"
    "================================================================================\n"
    "IDL: rosbag2_test_msgdefs/msg/BasicIdl\n"
    "module rosbag2_test_msgdefs {\n"
    "  module msg {\n"
    "    struct BasicIdl {\n"
    "        float x;\n"
    "    };\n"
    "  };\n"
    "};\n");
}

TEST(test_local_message_definition_source, no_crash_on_bad_name)
{
  LocalMessageDefinitionSource source;
  rosbag2_storage::MessageDefinition result;

  // The following type names are not valid, but it should not crash
  EXPECT_NO_THROW(
  {
    // The typename without preceding package name
    result = source.get_full_text_ext("/msg/String", "/msg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");

  EXPECT_NO_THROW(
  {
    // Missing the actual type name after format specifier
    result = source.get_full_text_ext("std_msgs/msg/", "/msg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");

  EXPECT_NO_THROW(
  {
    // Missing package name before the first slash
    result = source.get_full_text_ext("/String", "/msg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");

  EXPECT_NO_THROW(
  {
    // Hyphens are not allowed in package names
    result = source.get_full_text_ext("std-msgs/String", "/msg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");

  EXPECT_NO_THROW(
  {
    // File extensions are not allowed
    result = source.get_full_text_ext("std_msgs/String.msg", "/msg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");
}

TEST(test_local_message_definition_source, gracefully_handle_unknown_msg)
{
  LocalMessageDefinitionSource source;
  rosbag2_storage::MessageDefinition result;
  EXPECT_NO_THROW(
  {
    result = source.get_full_text_ext("rosbag2_test_msgdefs/msg/UnknownMessage",
      "/unknown_msg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");

  // No throw for not found message definition package name
  EXPECT_NO_THROW(
  {
    result = source.get_full_text_ext(
      "not_found_msgdefs_pkg/msg/UnknownMessage", "/not_found_msgdefs_pkg_topic");
  });
  EXPECT_EQ(result.encoding, "unknown");
}
