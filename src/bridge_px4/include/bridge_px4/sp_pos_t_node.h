
/**************************************************************************
  File Name    : setpoint_publisher_node.h
  Author       : JunEn Low
                 Multi-Robot Systems Lab (MSL), Stanford University
  Contact      : jelow@stanford.edu
  Create Time  : Nov 11, 2020.
  Description  : Publisher to Handle Various Setpoint Methods Over Mavlink
**************************************************************************/

#ifndef __SETPOINT_PUBLISHER_NODE_H__
#define __SETPOINT_PUBLISHER_NODE_H__

#include "ros/ros.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <geometry_msgs/PoseStamped.h>

#include <mavros_msgs/State.h>
#include <mavros_msgs/CommandTOL.h>

#include <Eigen/Dense>

using namespace Eigen;
using namespace std;

class SetpointPublisher {
private:
   // ROS variables
   ros::Publisher     pose_sp_pub;
   ros::Subscriber    state_sub;
   ros::Subscriber    pose_sub;
   ros::ServiceClient land_client;

   // Trajectory Variables
   MatrixXd    traj = MatrixXd::Zero(5,30);
   
   // Quad State Variables
   mavros_msgs::State         mode_curr;
   geometry_msgs::PoseStamped pose_curr;

   // Setpoint Variables
   enum sp_stream_status {
      SP_STREAM_READY,
      SP_STREAM_ACTIVE,
      SP_STREAM_COMPLETE
   } sp_status;
   geometry_msgs::PoseStamped pose_sp;

   // Counters and Time Variables
   int count_main;
   int count_traj;
   int N_traj;

   ros::Time   t_start; 
   ros::Time   t_traj;

public:
   // Constructor
   SetpointPublisher(ros::NodeHandle *nh, const std::string& traj_name);

   // Last Request Check
   bool last_req_check();

   // Telemetry Function(s)
   void state_cb(const mavros_msgs::State::ConstPtr& msg);
   void pose_cb(const geometry_msgs::PoseStamped::ConstPtr& msg);

   // Setpoint Function(s)
   void update_setpoint();

   MatrixXd load_trajectory(const std::string& input);

};

#endif
