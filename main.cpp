#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <thread>
#include <cmath>

#include <unistd.h>

#include <opencv2/opencv.hpp>
#include "config.hpp"
#include "serial_port.hpp"
#include "command_gen.hpp"
#include "key_input.hpp"
#include "motion.hpp"
#include "odometry.hpp"
#include "monitor.hpp"

//#define DEBUG_PRINT
//#define KEY_CONTROL	    

char key;

std::vector<char> key_buf;

int main(int argc, char *argv[])
{
    float camera_theta = 0;

    Odometry odometry(&camera_theta);
    odometry.Start();

    KeyInput keyInput;
    keyInput.Start();

    SerialPort khr_port(KHR_SERIAL_PORT);

    Motion motion(khr_port);
    std::map<int, int> dest;
    motion.Init(dest);
    motion.Move(dest, 10);
    sleep(1);

    motion.Clear(dest);
    motion.Grub(dest);
    motion.Move(dest, 10);
    sleep(1);

    std::map<int, Position> marker_map;
    Position goal;
    std::thread control_th = std::thread(
	[&]{
#ifdef KEY_CONTROL	    
	    while(1){
	    marker_map = odometry.getMap();
#ifdef DEBUG_PRINT
	    std::cout << marker_map[20].x << "\t"
		      << marker_map[20].y << "\t"
		      << marker_map[20].theta << "\t"
		      << atan2(-marker_map[20].x, marker_map[20].y) << "\t";
#endif

	    float target_theta = atan2(-marker_map[20].x, marker_map[20].y) - camera_theta;
	    motion.Clear(dest);

	    switch(keyInput.GetKey())
	    {
	case 'w':
#ifdef DEBUG_PRINT
	    std::cout << "forward";
#endif
	    motion.Forward(dest);
	    break;
	case 'a':
#ifdef DEBUG_PRINT
	    std::cout << "left";
#endif
	    motion.SetHeadOffset(1500);
	    motion.Left(dest, camera_theta);
	    break;
	case 's':
#ifdef DEBUG_PRINT
	    std::cout << "backward";
#endif
	    motion.Backward(dest);
	    break;

	case 'd':
#ifdef DEBUG_PRINT
	    std::cout << "right";
#endif
	    motion.SetHeadOffset(-1500);
	    motion.Right(dest, camera_theta);
	    break;
	default :
#ifdef DEBUG_PRINT
	    std::cout << "none";
#endif
	    motion.None(dest, camera_theta);
	    //motion.Stab(dest, theta, omega);
	    break;
	}
		
	    if((target_theta > 0.1 || target_theta < -0.1) && odometry.isSet())
		motion.Head(dest, target_theta, camera_theta);
	    motion.Move(dest, 20);

#ifdef DEBUG_PRINT
	    std::cout << std::endl;
#endif
	    usleep(30000);	
	}
#else
	    while(1){
		motion.Clear(dest);
		marker_map = odometry.getMap();
		float target_theta = atan2(-marker_map[20].x, marker_map[20].y) - camera_theta;
		if((target_theta > 0.1 || target_theta < -0.1) && odometry.isSet())
		    motion.Head(dest, target_theta, camera_theta);

		//目標地点の算出
		float l = 0.5f;
		goal = {
		    marker_map[20].x + l * sin(-marker_map[20].theta),
		    marker_map[20].y - l * cos(-marker_map[20].theta),
		    0.0f
		};

		// if(marker_map[20].theta > 0.7){
		//     motion.SetHeadOffset(-1500);
		//     motion.Right(dest, camera_theta);
		// }else if(marker_map[20].theta < -0.7){
		//     motion.SetHeadOffset(1500);
		//     motion.Left(dest, camera_theta);
		// }else{
		//     motion.None(dest, camera_theta);
		// }

		motion.Move(dest, 20);
		usleep(20000);	
	    }
#endif
	}
	);

    Monitor monitor(&argc, argv, &marker_map, &camera_theta, &goal);

    monitor.Start();
    
    control_th.join();
}

