# Unscented Kalman Filter Project Starter Code
Self-Driving Car Engineer Nanodegree Program

---

## Dependencies

* cmake >= 3.5
 * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools]((https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make` 
   * On windows, you may need to run: `cmake .. -G "Unix Makefiles" && make`
4. Run it: `./UnscentedKF path/to/input.txt path/to/output.txt`. You can find
   some sample inputs in 'data/'.
    - eg. `./UnscentedKF ../data/obj_pose-laser-radar-synthetic-input.txt`

## Results
Results of running the program on the `obj_pose-laser-radar-synthetic-input.txt` file should give the following output:
 
```
 RMSE
0.0592061
0.0845685
 0.301408
 0.153066
``` 

As a note of comparison, here is the results of running the Extended Kalman Filter code against the same dataset
```
 RMSE
0.0972256
0.0853761
 0.450855
 0.439588
```

## Simulator
Output of the program was changed so that it outputs RMSE in the format used by the simulator.
The `kalman-tracker.py` which can be used together with the simulator is included.

These are the results as seen on the simulator, with lidar and radar data first:

![Lidar and Radar](https://github.com/bhatiarajesh/CarND-Unscented-Kalman-Filter-Project/raw/master/out/KalmanFilterVisualizationToolOutput.png)

While these are the results using only lidar:

![Lidar Only](https://github.com/bhatiarajesh/CarND-Unscented-Kalman-Filter-Project/raw/master/out/Visualization-With-LIDAR-Only.png)

And only radar:

![Radar Only](https://github.com/bhatiarajesh/CarND-Unscented-Kalman-Filter-Project/raw/master/out/Visualization-With-RADAR-Only.png)

## Conclusion

I ensured that the code compiles with make. I collected the positions that my algorithm outputs and compared them to ground truth data. My px, py, vx, and vy RMSE was less than or equal to the values [.09, .10, .40, .30].My algorithm uses the first measurements to initialize the state vectors and covariance matrices. Upon receiving a measurement after the first, my algorithm predicted the object position to the current timestamp and then updated the prediction using the new measurement. The longitudinal and yaw acceleration noise parameters were tuned. The Scented Kalman Filter algorithm handles both radar and lidar measurements. I also experimented to see the impact of using either LIDAR or RADAR data and analyzed the RMSE. The fused LIDAR and RADAR values were found to be the best in terms of all the state related parameters. (px, py, vx and vy). I could have further changed the noise values to review the impact on RMSE. The 2-D Unity kalman filer simulator was used to study the LIDAR/RADAR and combined sensor fusion effects to track results.

In conclusion, the Unscented Kalman filter have the following abilities - 
* Able to take noisy measurement data as input and provide a smooth position and velocity estimation of dynamic objects
* Able to provide an estimation of the orientation and yaw rate of other vehicles using sensor data
* Able to provide information on how precise the result is by providing a covariance matrix for every estimation. The consistency check (via NIS calculations) shows how the covariance matrix is reliable.


## Project Instructions and Rubric

Note: regardless of the changes you make, your project must be buildable using
cmake and make!

More information is only accessible by people who are already enrolled in Term 2
of CarND. If you are enrolled, see [the project resources page](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/0949fca6-b379-42af-a919-ee50aa304e6a/lessons/f758c44c-5e40-4e01-93b5-1a82aa4e044f/concepts/382ebfd6-1d55-4487-84a5-b6a5a4ba1e47)
for instructions and the project rubric.

