#pragma once

#include <unordered_map>

#include "plotjuggler_msgs/msg/statistics_names.hpp"
#include "plotjuggler_msgs/msg/statistics_values.hpp"

#include "ros2_parser.h"
#include "fmt/format.h"
#include "header_msg.h"


static std::unordered_map<uint32_t, std::vector<std::string>> _stored_pj_statistics_names;

using plotjuggler_msgs::msg::StatisticsNames;
using plotjuggler_msgs::msg::StatisticsValues;

class StatisticsNamesParser : public BuiltinMessageParser<StatisticsNames>
{
public:
  StatisticsNamesParser(const std::string& topic_name, PJ::PlotDataMapRef& plot_data)
    : BuiltinMessageParser<StatisticsNames>(topic_name, plot_data)
  {
  }

  void parseMessageImpl(const StatisticsNames& msg, double& timestamp) override
  {
    _stored_pj_statistics_names.insert({ msg.names_version, msg.names });
  }
};

//-----------------------------------------------------
class StatisticsValuesParser : public BuiltinMessageParser<StatisticsValues>
{
public:
  StatisticsValuesParser(const std::string& topic_name, PJ::PlotDataMapRef& plot_data)
    : BuiltinMessageParser<StatisticsValues>(topic_name, plot_data)
    , _header_parser(topic_name + "/header", plot_data)
  {
  }

  void parseMessageImpl(const StatisticsValues& msg, double& timestamp) override
  {
    auto& values = _data[msg.names_version];

    _header_parser.parse(msg.header, timestamp, _use_header_stamp);

    auto names_it = _stored_pj_statistics_names.find(msg.names_version);
    if (names_it == _stored_pj_statistics_names.end())
    {
      return;  // missing vocabulary
    }
    const auto& names = names_it->second;

    if (msg.values.size() != names.size())
    {
      return;  // weird... skip
    }

    for (size_t index = 0; index < msg.values.size(); index++)
    {
      const auto& name = names[index];
      if (index >= values.size())
      {
        values.emplace_back(&getSeries(fmt::format("{}/{}",_topic_name, name)));
      }
      values[index]->pushBack({ timestamp, msg.values[index] });
    }
  }

private:
  std::unordered_map<uint32_t, std::vector<PJ::PlotData*>> _data;
  HeaderMsgParser _header_parser;
};
