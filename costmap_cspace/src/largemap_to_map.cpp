/*
 * Copyright (c) 2018, the neonavigation authors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ros/ros.h>
#include <nav_msgs/OccupancyGrid.h>
#include <tf/transform_listener.h>
#include <tf/transform_datatypes.h>

#include <string>

class LargeMapToMapNode
{
private:
  ros::NodeHandle pnh_;
  ros::NodeHandle nh_;
  ros::Publisher pub_map_;
  ros::Subscriber sub_largemap_;
  ros::Timer timer_;

  nav_msgs::OccupancyGrid::ConstPtr large_map_;
  tf::TransformListener tfl_;

  std::string robot_frame_;

  int width_;

public:
  LargeMapToMapNode()
    : pnh_("~")
    , nh_("")
  {
    pnh_.param("robot_frame", robot_frame_, std::string("base_link"));

    pub_map_ = pnh_.advertise<nav_msgs::OccupancyGrid>("map", 1, true);
    sub_largemap_ = nh_.subscribe("map", 2, &LargeMapToMapNode::cbLargeMap, this);

    pnh_.param("width", width_, 30);

    double hz;
    pnh_.param("hz", hz, 1.0);
    timer_ = nh_.createTimer(ros::Duration(1.0 / hz), &LargeMapToMapNode::cbTimer, this);
  }

private:
  void cbTimer(const ros::TimerEvent &event)
  {
    publishMap();
  }
  void cbLargeMap(const nav_msgs::OccupancyGrid::ConstPtr &msg)
  {
    large_map_ = msg;
  }
  void publishMap()
  {
    if (!large_map_)
      return;
    tf::StampedTransform trans;
    try
    {
      tfl_.lookupTransform(
          large_map_->header.frame_id, robot_frame_, ros::Time(0), trans);
    }
    catch (tf::TransformException &e)
    {
      return;
    }

    nav_msgs::OccupancyGrid map;
    map.header.frame_id = large_map_->header.frame_id;
    map.header.stamp = ros::Time::now();
    map.info = large_map_->info;
    map.info.width = width_;
    map.info.height = width_;

    const auto pos = trans.getOrigin();
    const float x = static_cast<int>(pos.x() / map.info.resolution) * map.info.resolution;
    const float y = static_cast<int>(pos.y() / map.info.resolution) * map.info.resolution;
    map.info.origin.position.x = x - map.info.width * map.info.resolution * 0.5;
    map.info.origin.position.y = y - map.info.height * map.info.resolution * 0.5;
    map.info.origin.position.z = 0.0;
    map.info.origin.orientation.w = 1.0;
    map.data.resize(width_ * width_);

    const int gx =
        lroundf((map.info.origin.position.x - large_map_->info.origin.position.x) / map.info.resolution);
    const int gy =
        lroundf((map.info.origin.position.y - large_map_->info.origin.position.y) / map.info.resolution);

    for (int y = gy; y < gy + width_; ++y)
    {
      for (int x = gx; x < gx + width_; ++x)
      {
        const size_t addr = (y - gy) * width_ + (x - gx);
        const size_t addr_large = y * large_map_->info.width + x;
        if (x < 0 || y < 0 ||
            x >= static_cast<int>(large_map_->info.width) ||
            y >= static_cast<int>(large_map_->info.height))
        {
          map.data[addr] = -1;
        }
        else
        {
          map.data[addr] = large_map_->data[addr_large];
        }
      }
    }

    pub_map_.publish(map);
  }
};

int main(int argc, char **argv)
{
  ros::init(argc, argv, "largemap_to_map");

  LargeMapToMapNode conv;
  ros::spin();

  return 0;
}
