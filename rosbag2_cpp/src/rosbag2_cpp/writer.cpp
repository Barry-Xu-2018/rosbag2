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

#include "rosbag2_cpp/writer.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "rcutils/types/uint8_array.h"

#include "rosbag2_cpp/info.hpp"
#include "rosbag2_cpp/storage_options.hpp"
#include "rosbag2_cpp/writer_interfaces/base_writer_interface.hpp"

#include "rosbag2_storage/serialized_bag_message.hpp"
#include "rosbag2_storage/topic_metadata.hpp"

namespace rosbag2_cpp
{

Writer::Writer(std::unique_ptr<rosbag2_cpp::writer_interfaces::BaseWriterInterface> writer_impl)
: writer_impl_(std::move(writer_impl))
{}

Writer::~Writer()
{
  writer_impl_.reset();
}

void Writer::open(const std::string & uri)
{
  rosbag2_cpp::StorageOptions storage_options;
  storage_options.uri = uri;
  storage_options.storage_id = "sqlite3";
  storage_options.max_bagfile_size = 0;  // default
  storage_options.max_cache_size = 0;  // default

  rosbag2_cpp::ConverterOptions converter_options{};

  return open(storage_options, converter_options);
}

void Writer::open(
  const StorageOptions & storage_options, const ConverterOptions & converter_options)
{
  writer_impl_->open(storage_options, converter_options);
}

void Writer::create_topic(const rosbag2_storage::TopicMetadata & topic_with_type)
{
  writer_impl_->create_topic(topic_with_type);
}

void Writer::remove_topic(const rosbag2_storage::TopicMetadata & topic_with_type)
{
  writer_impl_->remove_topic(topic_with_type);
}

void Writer::write(std::shared_ptr<rosbag2_storage::SerializedBagMessage> message)
{
  writer_impl_->write(message);
}

void Writer::write(
  const rclcpp::SerializedMessage & message,
  const std::string & topic,
  const rclcpp::Time & time)
{
  auto serialized_bag_message = std::make_shared<rosbag2_storage::SerializedBagMessage>();
  serialized_bag_message->topic_name = topic;
  serialized_bag_message->time_stamp = time.nanoseconds();

  // temporary store the payload in a shared_ptr.
  // add custom no-op deleter to avoid deep copying data.
  serialized_bag_message->serialized_data = std::shared_ptr<rcutils_uint8_array_t>(
    const_cast<rcutils_uint8_array_t *>(&message.get_rcl_serialized_message()),
    [](rcutils_uint8_array_t * /* data */) {});

  return write(serialized_bag_message);
}

}  // namespace rosbag2_cpp
