#include <bridge_px4/sp_pos_t_node.h>


SetpointPublisher::SetpointPublisher(ros::NodeHandle *nh, const std::string& traj_id)
{
    // ROS Initialization
    pose_sp_pub = nh->advertise<geometry_msgs::PoseStamped>("mavros/setpoint_position/local",1);
    state_sub   = nh->subscribe("mavros/state",1,&SetpointPublisher::state_cb,this);
    pose_sub    = nh->subscribe("/mavros/local_position/pose",1,&SetpointPublisher::pose_cb,this);
    land_client = nh->serviceClient<mavros_msgs::CommandTOL>("mavros/cmd/land");
    
    ROS_INFO("ROS Components Initialized");

    MatrixXd m_pose = load_trajectory(traj_id);
    N_traj = m_pose.cols();
    int N_x = m_pose.rows();

    traj.block(0,0,N_x,N_traj) = m_pose;

    // Setpoint Stream Initialization
    sp_status = SP_STREAM_READY;

    // Counter and Time Initialization
    count_main = 0;
    count_traj  = 0;
    ROS_INFO("Counters Initialized.");

    ROS_INFO("Setpoint Publisher Ready.");

}

void SetpointPublisher::state_cb(const mavros_msgs::State::ConstPtr& msg){
    mode_curr = *msg;

    if ((mode_curr.mode == "OFFBOARD") && sp_status == SP_STREAM_READY) {
        t_start = ros::Time::now();
        sp_status = SP_STREAM_ACTIVE;
        ROS_INFO("Trajectory Activated.");

        cout << "Heading to: \n" << pose_sp.pose.position << endl;
    } else if (mode_curr.mode != "OFFBOARD") {
        // Reset sp stream.
        sp_status = SP_STREAM_READY;
    }
}

void SetpointPublisher::pose_cb(const geometry_msgs::PoseStamped::ConstPtr& msg){
    pose_curr = *msg;
}

void SetpointPublisher::update_setpoint()
{   
    ros::Duration t_now = ros::Time::now() - t_start;

    if (sp_status == SP_STREAM_ACTIVE) {
        ros::Duration t_check = ros::Duration(traj(0,count_traj));

        if ( (count_traj < N_traj) && (t_now - t_check > ros::Duration(0)) ) {
            
            pose_sp.pose.position.x = traj(1,count_traj);
            pose_sp.pose.position.y = traj(2,count_traj);
            pose_sp.pose.position.z = traj(3,count_traj);
            
            double roll  = 0.0f;
            double pitch = 0.0f;
            double yaw   = traj(4,count_traj);
            
            double cy = cos(yaw * 0.5);
            double sy = sin(yaw * 0.5);
            double cp = cos(pitch * 0.5);
            double sp = sin(pitch * 0.5);
            double cr = cos(roll * 0.5);
            double sr = sin(roll * 0.5);
            
            pose_sp.pose.orientation.w = cr * cp * cy + sr * sp * sy;
            pose_sp.pose.orientation.x = sr * cp * cy - cr * sp * sy;
            pose_sp.pose.orientation.y = cr * sp * cy + sr * cp * sy;
            pose_sp.pose.orientation.z = cr * cp * sy - sr * sp * cy;

            count_traj++;

            cout << "Heading to: \n" << pose_sp.pose.position << endl;

        } else if (count_traj >= N_traj) {
            if (sp_status == SP_STREAM_ACTIVE) {
                ROS_INFO("Trajectory Complete. Attempting to Land.");
                mavros_msgs::CommandTOL srv_land;

                if (land_client.call(srv_land) && srv_land.response.success) {
                    ROS_INFO("land sent %d", srv_land.response.success);
                    sp_status = SP_STREAM_COMPLETE;
                }
            } else {
                // Trajectory Complete. Nothing to do here.
            }
        } else {
            // Keep publishing setpoint but don't need to update its value.
        }
    } else {
        pose_sp.pose.position.x = 0.0f;
        pose_sp.pose.position.y = 0.0f;
        pose_sp.pose.position.z = 0.0f;

        pose_sp.pose.orientation.w = 1.0f;
        pose_sp.pose.orientation.x = 0.0f;
        pose_sp.pose.orientation.y = 0.0f;
        pose_sp.pose.orientation.z = 0.0f;

        count_traj = 0;
    }

    pose_sp.header.stamp = ros::Time::now();
    pose_sp.header.seq   = count_main;
    pose_sp.header.frame_id = 1;
    count_main++;

    pose_sp_pub.publish(pose_sp);
}

MatrixXd SetpointPublisher::load_trajectory(const std::string& input)
{
    MatrixXd m_pose = MatrixXd::Zero(5,1);

    ifstream data(input);
    if (data.is_open()) {
        int rows = 0;
        int cols = 0;
        
        string line;
        vector<vector<double>> parsedCsv;

        while(getline(data,line))
        {
            stringstream lineStream(line);
            string cell;
            vector<double> parsedRow;
            while(getline(lineStream,cell,','))
            {
                parsedRow.push_back(stod(cell));
            
                if (rows == 0) {
                    cols += 1;
                }
            }
            parsedCsv.push_back(parsedRow);
            rows += 1;
        }
        m_pose.resize(rows,cols);

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols ; j++) {
                m_pose(i,j) = parsedCsv[i][j];
            }
        }
    } else {
        cout << "Trajectory does not exist. Defaulting to Origin." << endl;
    }
    
    return m_pose;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "setpoint_publisher_node");

    string traj_id;
    ros::param::get("~traj_id", traj_id);
    
    ros::NodeHandle nh;

    SetpointPublisher sp = SetpointPublisher(&nh,traj_id);

    ros::Rate rate(50);
    while(ros::ok()){
        sp.update_setpoint();
        
        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
