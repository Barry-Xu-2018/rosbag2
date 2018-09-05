// Copyright 2018, Bosch Software Innovations GmbH.
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

#include <gmock/gmock.h>

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include "../../src/rosbag2/rosbag2_node.hpp"
#include "../../src/rosbag2/typesupport_helpers.hpp"
#include "test_memory_management.hpp"

using namespace ::testing;  // NOLINT

class RosBag2NodeFixture : public Test
{
public:
  RosBag2NodeFixture()
  {
    node_ = std::make_shared<rosbag2::Rosbag2Node>("rosbag2");
  }

  static void SetUpTestCase()
  {
    rclcpp::init(0, nullptr);
  }

  static void TearDownTestCase()
  {
    rclcpp::shutdown();
  }

  std::vector<std::string> subscribe_raw_messages(
    size_t expected_messages_number, const std::string & topic_name, const std::string & type)
  {
    std::vector<std::string> messages;
    size_t counter = 0;
    auto subscription = node_->create_generic_subscription(topic_name, type,
        [this, &counter, &messages](std::shared_ptr<rmw_serialized_message_t> message) {
          auto string_message = memory_management_
          .deserialize_message<std_msgs::msg::String>(message);
          messages.push_back(string_message->data);
          counter++;
        });

    while (counter < expected_messages_number) {
      rclcpp::spin_some(node_);
    }
    return messages;
  }

  std::shared_ptr<rcutils_char_array_t> serialize_string_message(std::string message)
  {
    auto string_message = std::make_shared<std_msgs::msg::String>();
    string_message->data = message;
    return memory_management_.serialize_message(string_message);
  }

  test_helpers::TestMemoryManagement memory_management_;
  std::shared_ptr<rosbag2::Rosbag2Node> node_;
};


TEST_F(RosBag2NodeFixture, publisher_and_subscriber_work)
{
  // We currently publish more messages because they can get lost
  std::vector<std::string> test_messages = {"Hello World", "Hello World"};
  std::string topic_name = "string_topic";
  std::string type = "std_msgs/String";

  auto publisher = node_->create_generic_publisher(topic_name, type);

  auto subscriber_future_ = std::async(std::launch::async, [this, topic_name, type] {
        return subscribe_raw_messages(1, topic_name, type);
      });

  for (const auto & message : test_messages) {
    publisher->publish(serialize_string_message(message));
    // This is necessary because sometimes, the subscriber is initialized very late
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  auto subscribed_messages = subscriber_future_.get();
  EXPECT_THAT(subscribed_messages, SizeIs(Not(0)));
  EXPECT_THAT(subscribed_messages[0], StrEq("Hello World"));
}