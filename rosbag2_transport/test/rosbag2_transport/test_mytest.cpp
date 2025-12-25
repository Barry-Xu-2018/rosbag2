#include <gmock/gmock.h>
#include <memory>
#include <utility>
#include <vector>
#include <rclcpp/executors.hpp>
#include "rosbag2_transport/player.hpp"
#include "rosgraph_msgs/msg/clock.hpp"
#include "test_msgs/message_fixtures.hpp"
#include "mock_player.hpp"
#include "rosbag2_play_test_fixture.hpp"

using namespace ::testing;  // NOLINT

namespace
{
rcutils_duration_value_t period_for_frequency(double frequency)
{
  return static_cast<rcutils_duration_value_t>(RCUTILS_S_TO_NS(1) / frequency);
}
}  // namespace

class ClockPublishFixture : public RosBag2PlayTestFixture
{
public:
  void run_test()
  {
    auto topic_types = std::vector<rosbag2_storage::TopicMetadata>{
        {1u, "topic1", "test_msgs/BasicTypes", "", {}, ""},
    };
    std::vector<std::shared_ptr<rosbag2_storage::SerializedBagMessage>> messages;
    for (size_t i = 0; i < messages_to_play_; i++) {
      auto message = get_messages_basic_types()[0];
      message->int32_value = static_cast<int32_t>(i);
      messages.push_back(
          serialize_test_message("topic1", milliseconds_between_messages_ * i, message));
    }
    auto prepared_mock_reader = std::make_unique<MockSequentialReader>();
    prepared_mock_reader->prepare(messages, topic_types);
    auto reader = std::make_unique<rosbag2_cpp::Reader>(std::move(prepared_mock_reader));
    auto player = std::make_shared<MockPlayer>(std::move(reader), storage_options_, play_options_);
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(player);
    auto spin_thread = std::thread(
        [&exec]() {
          exec.spin();
        });
    sub_->add_subscription<test_msgs::msg::BasicTypes>("/topic1", messages.size());
    sub_->add_subscription<rosgraph_msgs::msg::Clock>(
        "/clock", expected_clock_messages_, rclcpp::ClockQoS());
    ASSERT_TRUE(
        sub_->spin_and_wait_for_matched(player->get_list_of_publishers(), std::chrono::seconds(30)));

    auto await_received_messages = sub_->spin_subscriptions();
    
    // Modify play to catch exceptions and verify playback starts correctly
    try {
      player->play();
      player->wait_for_playback_to_start();  // Ensure playback starts correctly
    } catch (const std::exception &e) {
      FAIL() << "Exception during playback start: " << e.what();
    }
    
    // Modify to ensure wait_for_playback_to_finish doesn't block indefinitely
    ASSERT_TRUE(player->wait_for_playback_to_finish(std::chrono::seconds(30)));
    
    await_received_messages.get();
    exec.cancel();
    spin_thread.join();
    
    auto received_clock = sub_->get_received_messages<rosgraph_msgs::msg::Clock>("/clock");
    ASSERT_THAT(received_clock, SizeIs(Ge(expected_clock_messages_)));
    if (play_options_.clock_publish_frequency > 0.f) {
      const auto expect_clock_delta =
          period_for_frequency(play_options_.clock_publish_frequency) * play_options_.rate;
      const auto allowed_error = static_cast<rcutils_duration_value_t>(
          expect_clock_delta * error_tolerance_);
      const auto start_message = 2;
      for (size_t i = start_message; i < expected_clock_messages_ - 1; i++) {
        auto current = rclcpp::Time(received_clock[i]->clock).nanoseconds();
        auto next = rclcpp::Time(received_clock[i + 1]->clock).nanoseconds();
        auto delta = next - current;
        auto error = std::abs(delta - expect_clock_delta);
        EXPECT_LE(error, allowed_error) << "Message was too far from next: " << i;
      }
    }
  }

  size_t messages_to_play_ = 10;
  const int64_t milliseconds_between_messages_ = 50;
  const double error_tolerance_ = 0.3;
  size_t expected_clock_messages_ = 6;
};

TEST_F(ClockPublishFixture, test)
{
  play_options_.clock_publish_frequency = 20;
  run_test();
}