#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>

#include <sensor_msgs/PointCloud2.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <laser_geometry/laser_geometry.h>
#include <tf/transform_listener.h>
#include <pcl/io/io.h>
#include <math.h>

class LidarGeofenceClass
{
public:

    ros::NodeHandle nh;
    ros::Subscriber scan_sub;

    //lines extraction
    ros::Publisher line_marker_pub;
    
    //publisher for points after filter
    ros::Publisher point_pub;
    
    //publisher type marker for points with minimum distance to every segment
    ros::Publisher point_marker_pub;
    
    // publisher type pc2 for points with minimum distance to every segment
    ros::Publisher points_dangerous_pub;
    
    //publisher for black area
    ros::Publisher black_area_pub;
    
    // publisher type pc2 for points with minimum distance to every segment
    ros::Publisher points_accelerate_pub;
    
    //points on acceleration
    pcl::PointCloud<pcl::PointXYZ> pc1;
    pcl::PointCloud<pcl::PointXYZ> pc2;
    pcl::PointCloud<pcl::PointXYZ> pc3;
    pcl::PointCloud<pcl::PointXYZ> points_accelerate;

    //record the points after filter
    pcl::PointCloud<pcl::PointXYZ> points_out;
   // std::vector<double> dist_list;
   long unsigned int n_points_out;
   
   //line extraction of points 
   std::vector< std::pair<long unsigned int, long unsigned int> > segment_list;
  
   

    // transform laser scan to points
    laser_geometry::LaserProjection projector;
    //tf::TransformListener listener;


    visualization_msgs::Marker line_marker;
    visualization_msgs::MarkerArray lines;
    
    //points with minimum distance to evevry segment
    visualization_msgs::Marker point_marker;
    
    visualization_msgs::Marker black_circle_marker;
    
    double dis_min;
    double dis_max;
    double epsi;
    int number_continue;
    LidarGeofenceClass():
	    nh("~")
    {
        scan_sub= nh.subscribe("/scan", 1, &LidarGeofenceClass::scanCallBack,this);
        line_marker_pub = nh.advertise<visualization_msgs::MarkerArray>("/lidar_line_marker",1);
        point_marker_pub = nh.advertise<visualization_msgs::Marker>("/lidar_points_marker",1);
        black_area_pub = nh.advertise<visualization_msgs::Marker>("/lidar_circle_marker",1);
        point_pub = nh.advertise<sensor_msgs::PointCloud2>("/lidar_cpt",1);
       points_dangerous_pub = nh.advertise<sensor_msgs::PointCloud2>("/lidar_points_dangerous", 3);
       points_accelerate_pub = nh.advertise<sensor_msgs::PointCloud2>("/lidar_points_accelerate",1);
       
       
	// blue line with 1cm width
        black_circle_marker.header.frame_id = point_marker.header.frame_id = line_marker.header.frame_id = "/laser";
        black_circle_marker.pose.orientation.w =  point_marker.pose.orientation.w =line_marker.pose.orientation.w = 1.0;
        black_circle_marker.action = point_marker.action = line_marker.action = visualization_msgs::Marker::ADD;
        black_circle_marker.lifetime = point_marker.lifetime = line_marker.lifetime = ros::Duration();
	
	
	line_marker.type = visualization_msgs::Marker::LINE_LIST;
	point_marker.type =  visualization_msgs::Marker::POINTS;
	black_circle_marker.type = visualization_msgs::Marker::CYLINDER;
	
	line_marker.id = 0;
	point_marker.id = 1;
	
	line_marker.scale.x = 0.002;
	line_marker.color.b = 1.0;
	line_marker.color.a = 1.0;
	
	point_marker.scale.x = 0.005; 
	point_marker.scale.y = 0.005;
	point_marker.color.r = 1.0;
	point_marker.color.a = 1.0;
	
	black_circle_marker.color.r = 0.2;
	black_circle_marker.color.b = 0.8;
	black_circle_marker.color.a = 1.0;
	
	
	
	nh.param("epsi",epsi,0.12);
	nh.param("dis_min",dis_min, 0.0);
	nh.param("dis_max",dis_max, 1.0);
	nh.param("number_continue", number_continue, 131);
	
	//initialize
	pc1.push_back(pcl::PointXYZ(0.0,0.0,0.0));
	pc2.push_back(pcl::PointXYZ(0.0,0.0,0.0));
	pc3.push_back(pcl::PointXYZ(0.0,0.0,0.0));


    }

  void scanCallBack(const sensor_msgs::LaserScan::ConstPtr& scan_in)
  {
	  //record time step
	  double start = ros::Time::now().toSec();

	  //record distance information
	 // std::vector<float> points_dis = scan_in->ranges;
		
	 //transform distance to point 		  
	  sensor_msgs::PointCloud2  msg_points_origin;
	  projector.projectLaser(*scan_in, msg_points_origin);

	  //save all the input points
	  pcl::PointCloud<pcl::PointXYZ> points_origin;
	// transform all the messages to pcl
	  pcl::fromROSMsg(msg_points_origin,points_origin);
	// number of all the points
	  long unsigned int number_points_in = points_origin.size();
          
	  
	   ROS_INFO("number points in %lu", number_points_in);

	  // filter the points ang keep those in the dangerous region
	  for(unsigned long int i =0; i < number_points_in; i++){
		  
		  float point_dis = sqrt(points_origin[i].x*points_origin[i].x + points_origin[i].y*points_origin[i].y);
		 if(point_dis >= dis_min && point_dis <= dis_max){
			  //ROS_INFO("debug0");

			 //coordiante information of every point
			  float x = points_origin[i].x;
			  float y = points_origin[i].y;
			  float z = points_origin[i].z;
			  points_out.push_back(pcl::PointXYZ(x,y,z));
                         /* if(i == 0){ 
                            ROS_INFO("%f, %f, %f, %f ", x, y, z, point_dis);
			} */
                }
	  }

          //to convert all the points to PointCloud2
	  sensor_msgs::PointCloud2 out_msg;
	  
	  pcl::toROSMsg(points_out,out_msg);
	  out_msg.header.stamp= scan_in->header.stamp;
	  out_msg.header.frame_id=scan_in->header.frame_id;
	  out_msg.header.seq=scan_in->header.seq;
	  //publish it
	  point_pub.publish(out_msg);
          n_points_out = points_out.size();
	 // split_merge(points_out,points_out[0], points_out[n_points_out-1]);
          
          
          pcl::PointCloud<pcl::PointXYZ> points_dangerous;
          if(n_points_out > 0){
	  split_merge(points_out, n_points_out);
	   ROS_INFO("number of segments %lu", segment_list.size());
	  points_dangerous = points_mindis_to_seg(segment_list, points_out);
          }
	
	  line_marker_pub.publish(lines);
	  point_marker_pub.publish(point_marker);
	  
	  //PUBLISH THE DANGEROUS POINTS
	  sensor_msgs::PointCloud2 out_point_msg;
	  
	  
	  long unsigned int number_points_dang = points_dangerous.size();
          if(number_points_dang <= 0){
            ROS_ERROR("No dangerous point found."); 
            return;
          }
	  
	 // ROS_INFO("number of points input: %lu",points_dangerous.size());
	  
	  
	  //points_accelerate 
	 /* points_acceleration(points_out,n_points_out);
	  sensor_msgs::PointCloud2 out_points_acceleration_msg;
	  pcl::toROSMsg(points_accelerate,out_points_acceleration_msg);
	  out_points_acceleration_msg.header.stamp= scan_in->header.stamp;
	  out_points_acceleration_msg.header.frame_id=scan_in->header.frame_id;
	  out_points_acceleration_msg.header.seq=scan_in->header.seq;	
	  points_accelerate_pub.publish(out_points_acceleration_msg);
	  ROS_INFO("number pl1 %lu", pc1.size());
	  ROS_INFO("number pl2 %lu", pc2.size());
	  ROS_INFO("number pl3 %lu", pc3.size());  */
	   pcl::toROSMsg(points_dangerous,out_point_msg);
	     out_point_msg.header.stamp= scan_in->header.stamp;
	     out_point_msg.header.frame_id=scan_in->header.frame_id;
	     out_point_msg.header.seq=scan_in->header.seq;	 
             points_dangerous_pub.publish(out_point_msg);
             points_dangerous.clear();
             lines.markers.clear();
	     points_out.clear();
	   //points_accelerate.clear();
	  
	  line_marker.points.clear();
	  point_marker.points.clear();
	  segment_list.clear();

	 
	  double end = ros::Time::now().toSec();
	  double duree_calcul=(end-start);
	  //ROS_INFO("Total calculation time: %.2fms",duree_calcul*1000);
	  //ROS_INFO("number of points: %lu",n_points_out);
  } 

  void points_acceleration(const pcl::PointCloud<pcl::PointXYZ> points_in, long unsigned int number_points_in){
	  
	  //enregistrer all the points in three cycles
	      pc3 = pc2;
	      pc2 = pc1;
	      pc1 = points_in;
		  
		  long unsigned int size_min = pc1.size();
		  if(pc2.size()< size_min){
			  size_min = pc2.size();
		  }
		  if(pc3.size() < size_min){
		  		  size_min = pc1.size();
		  	  }
		  
		  for(long unsigned int i = 0; i < size_min; i++){
			  double dis_t_ini,dis_t_mid, dis_t_rec;
			  dis_t_ini = sqrt(pc3[i].x*pc3[i].x + pc3[i].y*pc3[i].y);
			  dis_t_mid = sqrt(pc2[i].x*pc2[i].x + pc2[i].y*pc2[i].y);
			  dis_t_rec = sqrt(pc1[i].x*pc1[i].x + pc1[i].y*pc1[i].y);
			  if(dis_t_mid - dis_t_rec > dis_t_ini - dis_t_mid && dis_t_mid - dis_t_rec > 0){
				  
			      float x = pc1[i].x;
				  float y = pc1[i].y;
				  float z = pc1[i].z;
				  points_accelerate.push_back(pcl::PointXYZ(x,y,z));
			  }
			  
		  }
	    
	  
  }

  void split_merge(pcl::PointCloud<pcl::PointXYZ> points, long unsigned number_points){

	  //list of segment and marker
	 // std::vector<geometry_msgs::Point> segment; 
	  //std::vector<std::vector<geometry_msgs::Point> > segment_list;
         
	  segment_list.push_back(std::make_pair(0,number_points -1));
	  
	  bool split = true;
	  
	  while(split){	 
		  split = false;
          
		  //std::vector<std::vector<geometry_msgs::Point> > segment_list_temp;		 
	  	  //ROS_INFO("debug1");
		  
		  std::vector<std::pair<long unsigned int, long unsigned int> > segment_list_temp;
		  
		  //std::vector<std::vector<int> > points_in_seg_list_temp;
		  //ROS_INFO("number segment %lu", segment_list.size());
		  
		  // for every segment
		  for(long unsigned int i =0; i < segment_list.size(); i++){
			  
			  std::pair<long unsigned int, long unsigned int> segment_temp1;
			  std::pair<long unsigned int, long unsigned int> segment_temp2;
			  //ROS_INFO("debug start for");
	                  //ROS_INFO("number points %lu", number_points);
              
              //continue if there is only one point
			/*  if(segment_list[i].second - segment_list[i].first < 1){
				  segment_list.erase(segment_list.begin()+i);
				  continue;
			  } */
               
			  /*long unsigned int number_points_seg = points_in_seg_list[i].size();
              if (number_points_seg < 2) {
		  points_in_seg_list.erase(points_in_seg_list.begin()+i);
		  continue;
              }*/
			 		  
			  double dis_to_line_max = 0.0;
			  unsigned int mark_of_dis_max = 0;
			  //start and end point for every segment
			 
			  geometry_msgs::Point point_start;
			  geometry_msgs::Point point_end;
			  
			  point_start.x = points[segment_list[i].first].x;
			  point_start.y = points[segment_list[i].first].y;
			  point_start.z = points[segment_list[i].first].z;
			  point_end.x = points[segment_list[i].second].x;
			  point_end.y = points[segment_list[i].second].y;
			  point_end.z = points[segment_list[i].second].z;

			  double a = point_end.y - point_start.y;
			  double b = -(point_end.x - point_start.x);
			  double c = -point_start.x*a - point_start.y*b;

			  
			 // ROS_INFO("debug start for 1");
			  //ROS_INFO("number points_seg %lu", number_points_seg);
			  
			  
			  //calculate the maximum distance
			  for(long unsigned int j = segment_list[i].first;j <= segment_list[i].second; j++){
				  double dis_to_line_j = fabs(a*points[j].x + b*points[j].y +c)/(a*a+b*b);
				  if(dis_to_line_j > dis_to_line_max){
					  dis_to_line_max = dis_to_line_j;
					  mark_of_dis_max = j;
					}		
			          
			  }

			  //ROS_INFO("debug start for 2");
			  if (dis_to_line_max <= epsi){
				  //line_marker.points.push_back(p1);
				  //line_marker.points.push_back(p2);
				  //lines.markers.push_back(line_marker);
                  
	  			 // ROS_INFO("debug if");

				  segment_list_temp.push_back(std::make_pair(segment_list[i].first, segment_list[i].second));	  
			  }
			  else{
			  //ROS_INFO("debug start for 3");
				  split =true;
	  			  //ROS_INFO("debug else");

				 
				  //split in two parts
				  
				  segment_list_temp.push_back(std::make_pair(segment_list[i].first, mark_of_dis_max));
				  segment_list_temp.push_back(std::make_pair(mark_of_dis_max,segment_list[i].second));
				  
			  }
		  }

		  segment_list.clear();
		  for(long unsigned int i =0 ; i < segment_list_temp.size(); i++){

			  segment_list.push_back(segment_list_temp[i]);
		  }
	  	        //  ROS_INFO("number new seg %lu", segment_list.size());
		  


	  }
	  for(int i=0; i< segment_list.size();i++){
		       geometry_msgs::Point marker_start;
		 	   geometry_msgs::Point marker_end;
		            		 	  
		 	  marker_start.x= points[segment_list[i].first].x;
		 	  marker_start.y= points[segment_list[i].first].y;
		 	  marker_start.z= points[segment_list[i].first].z;

		 	  marker_end.x = points[segment_list[i].second].x;
		 	  marker_end.y = points[segment_list[i].second].y;
		 	  marker_end.z = points[segment_list[i].second].z;
		 	  		
	  	  line_marker.points.push_back(marker_start);
		 line_marker.points.push_back(marker_end);
		 lines.markers.push_back(line_marker);
	  }
	  
  }

  //find the points with the minimum distance to every segment
  pcl::PointCloud<pcl::PointXYZ>  points_mindis_to_seg(std::vector<std::pair<long unsigned int, long unsigned int> > segment_list, pcl::PointCloud<pcl::PointXYZ> points){
	
	  pcl::PointCloud<pcl::PointXYZ> points_dangerous;
	  std::vector<std::pair<double, double> > points_dismin; 
	  
	  for(long unsigned int i= 0; i < segment_list.size(); i++){
		  double x1 = points[segment_list[i].first].x;
		  double y1 = points[segment_list[i].first].y;
		  double x2 = points[segment_list[i].second].x;
		  double y2 = points[segment_list[i].second].y;
		  
		  double diffX = x2-x1;
		  double diffY = y2-y1;
		  
		  if ((diffX == 0) && (diffY == 0))
		      {    
		       points_dismin.push_back(std::make_pair(x1, y1));	         
		      }
		  
		  else{
			  float t = ((- x1) *diffX + (- y1) * diffY) / (diffX * diffX + diffY * diffY);
		      if (t < 0)
		      {
		          //point is nearest to the first point i.e x1 and y1
		          diffX =  - x1;
		          diffY =  - y1;
		          
		          points_dismin.push_back(std::make_pair(x1, y1));
		          
		      }
		      else if (t > 1)
		      {
		          //point is nearest to the end point i.e x2 and y2
		          diffX =  - x2;
		          diffY =  - y2;
		          
		          points_dismin.push_back(std::make_pair(x2, y2));
		      }
		      else
		      {
		          //if perpendicular line intersect the line segment.
		          diffX =  - (x1 + t * diffX);
		          diffY =  - (y1 + t * diffY);
		          double m = (y2 - y1) / (x2- x1);
		          double b = y1 - (m * x1);
		          
		          double project_x = ( - m * b) / (m * m + 1);
		          double project_y = (  b) / (m * m + 1);
		          points_dismin.push_back(std::make_pair(project_x, project_y));     
		      }  
		  }
		  
	  }
	  
	  for(long unsigned int i = 0; i < points_dismin.size();i++){
		  geometry_msgs::Point point_marker_i;		  
		  point_marker_i.x = points_dismin[i].first;
		  point_marker_i.y = points_dismin[i].second;
		  point_marker_i.z = 0.0;
		  points_dangerous.push_back(pcl::PointXYZ(points_dismin[i].first,points_dismin[i].second, 0.0 ));  
		  point_marker.points.push_back(point_marker_i);	  
	  }
	  
	  return points_dangerous;
	  
  }
  
  
  
  void black_area_judge(pcl::PointCloud<pcl::PointXYZ> points_in){
	  std::vector<std::pair<int,int> > region_black;
	  int count_continue =0;
	  int start,end;
	  
	  long unsigned int number_points_in = points_in.size();
	  
	  for(long unsigned int i =0; i < number_points_in; i++){
		float point_dis = sqrt(points_in[i].x*points_in[i].x + points_in[i].y*points_in[i].y);
		  if(point_dis > 6){
			  if(count_continue ==0){
				  start = i;
			  }
			  count_continue ++;
		  }
		  else{
			  if(count_continue >= number_continue){
			  			  end = i;
			  			  region_black.push_back(std::make_pair(start,end));
			  		  }
			  count_continue = 0;
		  }
		  
	  }
	  
  }
    
    
};

int main (int argc, char** argv)
{
    ros::init(argc, argv, "lidar_geofence_node");

    LidarGeofenceClass filter;

    ros::Rate loop_rate(5);
    //unsigned int count =0;

  //  while (ros::ok()){
  //  	    ros::spinOnce();
	        //loop_rate.sleep();
	   // count++;
	   // ROS_INFO("Point cloud nb: %u",count);
//	    ROS_INFO("Number of clouds: %lu",filter.n_points_out);
 //   }
      ros::spin();
}

