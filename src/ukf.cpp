#include "ukf.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */

UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = .5; // Acceleration between -1 and +1 95% of time

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = .725;

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  //set state dimension
  n_x_ = 5;

  //set augmented dimension
  n_aug_ = 7;

  // Initially set to false, set to true in first call of ProcessMeasurement
  is_initialized_ = false;

  // Predicted sigma points matrix
  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);

  // Sigma point spreading parameter
  lambda_ = 3 - n_x_;

  // Weights of sigma points
  weights_ = VectorXd(2 * n_aug_ + 1);

  // The current NIS (consistency check) for laser
  NIS_laser_ = 0.0;
  
  // The current NIS (consistency check) for radar
  NIS_radar_ = 0.0;
}

UKF::~UKF() {}

/** ProcessMeasurement function that essentially implements an Initialization, Prediction and Update step that is common to all Kalman filters. The difference in the Unscented Kalman Filter (UKF) is that  * it uses non-linear equations differently than an extended  * Kalman Filter(EKF). This function also switches between lidar and radar measurements.
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  if (!is_initialized_) {
    if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
      //Convert radar from polar to cartesian coordinates and initialize state.
      double rho = meas_package.raw_measurements_.coeff(0); //rho
      double phi = meas_package.raw_measurements_.coeff(1); //phi
      //Initialize state x_
      x_ << rho * cos(phi), rho * sin(phi), 0, 0, 0;

    } else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {
      //Initialize state vector.
      x_ << meas_package.raw_measurements_.coeff(0), meas_package.raw_measurements_.coeff(1), 0, 0, 0;
    }
     //Initialize state covariance matrix
     P_ <<
        .6, 0, 0, 0, 0, // px
        0, .6, 0, 0, 0, // py
        0, 0, 6, 0, 0,  // v
        0, 0, 0, 7.5, 0,  // yaw
        0, 0, 0, 0, 0;  // yaw_d

     // done initializing
     time_us_ = meas_package.timestamp_;
     is_initialized_ = true;
     return;
    }
  
   // Calculate new elasped time and Time is measured in seconds
   double dt = (meas_package.timestamp_ - time_us_) / 1000000.0;    //dt - expressed in seconds
   time_us_ = meas_package.timestamp_;
   Prediction(dt);
  //std::cout << "Xsig_pred_ = " << std::endl << Xsig_pred_ << std::endl;
  
  // Update step based on Lidar or Radar measurement
  if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
    UpdateRadar(meas_package);
  } else {
    UpdateLidar(meas_package);
  }
}

/**
 * Estimate the object's location. Modify the state vector, x_. Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {

  //create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  //create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_,n_aug_);

  //create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);
  
  //create augmented mean state
  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;

  //create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_*std_a_;
  P_aug(6,6) = std_yawdd_*std_yawdd_;

  //create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  //create augmented sigma points
  Xsig_aug.col(0)  = x_aug;
  for (int i = 0; i < n_aug_; i++)
  {
    Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
  }

  //predict sigma points
  for (int i = 0; i< 2*n_aug_+1; i++)
  {
    //extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    //predicted state values
    double px_p, py_p;

    //avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    }
    else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    //add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
    yawd_p = yawd_p + nu_yawdd*delta_t;

    //write predicted sigma point into right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;
  }
  
   //set vector for weights
  double weight_0 = lambda_/(lambda_+n_aug_);
  weights_(0) = weight_0;
  for (int i=1; i<2*n_aug_+1; i++) {  //2n+1 weights
    double weight = 0.5/(n_aug_+lambda_);
    weights_(i) = weight;
  }

  //predict state mean
  x_.fill(0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {
    x_ = x_ + Xsig_pred_.col(i) * weights_(i);
  }

  //predict state covariance matrix
  P_.fill(0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
    while (x_diff(3) > M_PI) x_diff(3) -= 2. * M_PI;
    while (x_diff(3) < -M_PI) x_diff(3) += 2. * M_PI;
    P_ = P_ + weights_(i) * x_diff * x_diff.transpose();
  }
}
/**
 * Updates the state and the state covariance matrix using a laser measurement and calculate NIS for Lidar
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  int n_z = 2;
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; i++) { //2n+1 simga points
      // extract values for better readibility
     double p_x = Xsig_pred_(0, i);
     double p_y = Xsig_pred_(1, i);
     // measurement model
     Zsig(0, i) = p_x; //px
     Zsig(1, i) = p_y; //py
   }
   
   //mean predicted measurement
   VectorXd z_pred = VectorXd(n_z);
   z_pred.fill(0.0);
   for (int i = 0; i < 2 * n_aug_ + 1; i++) {
       z_pred = z_pred + weights_(i) * Zsig.col(i);
    }
    
    //measurement covariance matrix S
    MatrixXd S = MatrixXd(n_z, n_z);
    S.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) { //2n+1 simga points
       //residual
       VectorXd z_diff = Zsig.col(i) - z_pred;
       //angle normalization
       while (z_diff(1) > M_PI) z_diff(1) -= 2. * M_PI;
       while (z_diff(1) < -M_PI) z_diff(1) += 2. * M_PI;
       S = S + weights_(i) * z_diff * z_diff.transpose();
     }
    
     //add measurement noise covariance matrix
     MatrixXd R = MatrixXd(n_z, n_z);
     R << std_laspx_ * std_laspx_, 0,
        0, std_laspy_ * std_laspy_;
     S = S + R;
     
     VectorXd z = meas_package.raw_measurements_;
     VectorXd y = z - z_pred;
     // calculate laser NIS
     NIS_laser_ = (y).transpose() * S.inverse() * (y);

     //calculate cross correlation matrix
     MatrixXd Tc = MatrixXd(this->n_x_, n_z);
     Tc.fill(0.0);
     for (int i = 0; i < 2 * this->n_aug_ + 1; i++) { //2n+1 simga points
        //residual
        VectorXd z_diff = Zsig.col(i) - z_pred;
        //angle normalization
       while (z_diff(1) > M_PI) z_diff(1) -= 2. * M_PI;
       while (z_diff(1) < -M_PI) z_diff(1) += 2. * M_PI;
       // state difference
       VectorXd x_diff = this->Xsig_pred_.col(i) - this->x_;
       //angle normalization
       while (x_diff(3) > M_PI) x_diff(3) -= 2. * M_PI;
       while (x_diff(3) < -M_PI) x_diff(3) += 2. * M_PI;
       Tc = Tc + this->weights_(i) * x_diff * z_diff.transpose();
     }
 
     //Kalman gain K;
     MatrixXd K = Tc * S.inverse();
     //residual
     VectorXd z_diff = z - z_pred;

     //angle normalization
     while (z_diff(1) > M_PI) z_diff(1) -= 2. * M_PI;
     while (z_diff(1) < -M_PI) z_diff(1) += 2. * M_PI;

     //update state mean and covariance matrix
     this->x_ = this->x_ + K * z_diff;
     this->P_ = this->P_ - K * S * K.transpose();  
}

/**
 * Updates the state and the state covariance matrix using a radar measurement and calculate the radar NIS
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  //set measurement dimension, radar can measure r, phi, and r_dot
  int n_z = 3;

  //create example matrix with sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  //create example vector for mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  //create example matrix for predicted measurement covariance
  MatrixXd S = MatrixXd(n_z,n_z);
  //create example vector for incoming radar measurement
  VectorXd z = VectorXd(n_z);
  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(this->n_x_, n_z);
  
  //transform sigma points into measurement space
  for (int i = 0; i < 2 * this->n_aug_ + 1; i++) {  //2n+1 simga points

    // extract values for better readibility
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    // measurement model
    Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //r
    Zsig(1,i) = atan2(p_y,p_x);                                 //phi
    Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //r_dot
  }

  //mean predicted measurement
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; i++) {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  //measurement covariance matrix S
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<    std_radr_*std_radr_, 0, 0,
          0, std_radphi_*std_radphi_, 0,
          0, 0,std_radrd_*std_radrd_;
  S = S + R;
   

  z = meas_package.raw_measurements_;
  VectorXd y = z - z_pred;

  // calculate radar NIS
  NIS_radar_ = (y).transpose() * S.inverse() * (y);
  
  //calculate cross co relatiion matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * this->n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    // state difference
    VectorXd x_diff = this->Xsig_pred_.col(i) - this->x_;
    //angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    Tc = Tc + this->weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff = z - z_pred;

  //angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

  //update state mean and covariance matrix
  this->x_ = this->x_ + K * z_diff;
  this->P_ = this->P_ - K*S*K.transpose();
}
