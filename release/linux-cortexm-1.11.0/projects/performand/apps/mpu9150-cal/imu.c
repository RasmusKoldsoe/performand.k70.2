////////////////////////////////////////////////////////////////////////////
//
//  This file is part of linux-mpu9150
//
//  Copyright (c) 2013 Pansenti, LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>
//#include <fcntl.h>
#include <unistd.h>
#include <math.h>


#include "mpu9150/mpu9150.h"
#include "glue/linux_glue.h"
#include "local_defaults.h"

#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/logging.h"
#include "../Common/utils.h"

int set_cal(int mag, char *cal_file);
void set_config(double *val);
void read_loop(unsigned int sample_rate);
void print_fused_euler_angles(mpudata_t *mpu);
void log_fused_euler_angles(mpudata_t *mpu);
void print_fused_quaternion(mpudata_t *mpu);
void print_calibrated_accel(mpudata_t *mpu);
void print_calibrated_mag(mpudata_t *mpu);
void register_sig_handler();
void sigint_handler(int sig);

int done;
FILE *logfd;
int runtime_count,file_idx = 0;

//float x_avg=0,y_avg=0,z_avg=0;
//int mag_raw_x_avg=0,mag_raw_y_avg=0,mag_raw_z_avg=0;
//float acc_x_avg=0, acc_y_avg=0, acc_z_avg=0;
float gyro_x_avg=0, gyro_y_avg=0, gyro_z_avg=0;
//int acc_x[5];
//int acc_y[5];	
//int acc_z[5];
//int mag_raw_x[5];
//int mag_raw_y[5];	
//int mag_raw_z[5];
int time_s;
int time_ms;

float acc[] = {0,0,0};
float gyro[] = {0,0,0};
//float mag[] = {0,0,0}; 

#define WGYRO 5

void usage(char *argv_0)
{
	printf("\nUsage: %s [options]\n", argv_0);
	printf("  -b <i2c-bus>          The I2C bus number where the IMU is. The default is 1 to use /dev/i2c-1.\n");
	printf("  -s <sample-rate>      The IMU sample rate in Hz. Range 2-50, default 5.\n");
	printf("  -y <yaw-mix-factor>   Effect of mag yaw on fused yaw data.\n");
	printf("                           0 = gyro only\n");
	printf("                           1 = mag only\n");
	printf("                           > 1 scaled mag adjustment of gyro data\n");
	printf("                           The default is 4.\n");
	printf("  -a <accelcal file>    Path to accelerometer calibration file. Default is ./accelcal.txt\n");
	printf("  -m <magcal file>      Path to mag calibration file. Default is ./magcal.txt\n");
	printf("  -v                    Verbose messages\n");
	printf("  -h                    Show this help\n");

	printf("\nExample: %s -b3 -s20 -y10\n\n", argv_0);
	
	exit(1);
}

int main(int argc, char **argv)
{
	int opt, len;
	int i2c_bus = DEFAULT_I2C_BUS;
	int sample_rate = DEFAULT_SAMPLE_RATE_HZ;
	int yaw_mix_factor = DEFAULT_YAW_MIX_FACTOR;
	int verbose = 0;
	char *mag_cal_file = NULL;
	char *accel_cal_file = NULL;

	while ((opt = getopt(argc, argv, "b:s:y:a:m:vh")) != -1) {
		switch (opt) {
		case 'b':
			i2c_bus = strtoul(optarg, NULL, 0);
			
			if (errno == EINVAL)
				usage(argv[0]);
			
			if (i2c_bus < MIN_I2C_BUS || i2c_bus > MAX_I2C_BUS)
				usage(argv[0]);

			break;
		
		case 's':
			sample_rate = strtoul(optarg, NULL, 0);
			
			if (errno == EINVAL)
				usage(argv[0]);
			
			if (sample_rate < MIN_SAMPLE_RATE || sample_rate > MAX_SAMPLE_RATE)
				usage(argv[0]);

			break;

		case 'y':
			yaw_mix_factor = strtoul(optarg, NULL, 0);
			
			if (errno == EINVAL)
				usage(argv[0]);
			
			if (yaw_mix_factor < 0 || yaw_mix_factor > 100)
				usage(argv[0]);

			break;

		case 'a':
			len = 1 + strlen(optarg);

			accel_cal_file = (char *)malloc(len);

			if (!accel_cal_file) {
				perror("malloc");
				exit(1);
			}

			strcpy(accel_cal_file, optarg);
			break;

		case 'm':
			len = 1 + strlen(optarg);

			mag_cal_file = (char *)malloc(len);

			if (!mag_cal_file) {
				perror("malloc");
				exit(1);
			}

			strcpy(mag_cal_file, optarg);
			break;

		case 'v':
			verbose = 1;
			break;

		case 'h':
		default:
			usage(argv[0]);
			break;
		}
	}

	register_sig_handler();

	mpu9150_set_debug(verbose);

	printf("IMU sample rate: %d Hz",sample_rate);
	if (mpu9150_init(i2c_bus, sample_rate, yaw_mix_factor)) {
		fprintf(stderr, "[IMU_daemon] ERROR Initialisation failed.\n");
		return 0;
	}

	set_cal(0, accel_cal_file);
	set_cal(1, mag_cal_file);

	if (accel_cal_file)
		free(accel_cal_file);

	if (mag_cal_file)
		free(mag_cal_file);

	read_loop(sample_rate);

	mpu9150_exit();

	return 0;
}

double square(double x) {
	return x*x;
}

double absoluteValue3DVector(double *vector) {
	return sqrt( square(vector[VEC3_X]) + square(vector[VEC3_Y]) + square(vector[VEC3_Z]) );
}

void tomappedMemXMLPacket(mpudata_t *mpu, h_mmapped_file *mapped_file, struct timespec spec, log_t *sdLog) {
	char buffer[250];
	int length;
	format_timespec(buffer, &spec);
	if ((length = snprintf(buffer+strlen(buffer), 250, ",%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%0.2f,%0.2f,%3.2f,%3.2f,%d.%d\n",
		mpu->rawMag[VEC3_X],
		mpu->rawMag[VEC3_Y],
		mpu->rawMag[VEC3_Z],
		time_s,time_ms,

		mpu->rawAccel[VEC3_X],
		mpu->rawAccel[VEC3_Y],
		mpu->rawAccel[VEC3_Z],
		time_s,time_ms,

		mpu->rawGyro[VEC3_X],
		mpu->rawGyro[VEC3_Y],
		mpu->rawGyro[VEC3_Z],
		time_s,time_ms,

		mpu->pitch,
		mpu->roll,
		mpu->heading,
		mpu->ema_heading,
		time_s,time_ms)) > 0) {
				
		mm_append(buffer, mapped_file);

//		write_log_file("imu", runtime_count, &file_idx, buffer);
		append_log(sdLog, buffer, length);
	}
	return;

}

void tomappedMemXMLPacket_RAW(mpudata_t *mpu, h_mmapped_file *mapped_file, struct timespec spec, log_t *sdLog) {
	char buffer[600];
	int length;

	format_timespec(buffer, &spec);
	if ((sprintf(buffer+strlen(buffer),",%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%0.2f,%0.2f,%3.2f,%3.2f,%d.%d\n",
		mpu->rawMag[VEC3_X],
		mpu->rawMag[VEC3_Y],
		mpu->rawMag[VEC3_Z],
		time_s,time_ms,

		mpu->rawAccel[VEC3_X],
		mpu->rawAccel[VEC3_Y],
		mpu->rawAccel[VEC3_Z],
		time_s,time_ms,

		mpu->rawGyro[VEC3_X],
		mpu->rawGyro[VEC3_Y],
		mpu->rawGyro[VEC3_Z],
		time_s,time_ms,

		mpu->pitch,
		mpu->roll,
		mpu->heading,
		mpu->ema_heading,
		time_s,time_ms)) > 0) {
				
		mm_append(buffer, mapped_file);
		//write_log_file("imu", runtime_count, &file_idx, buffer);
		append_log(sdLog, buffer, length);
	}
	return;

/*	char buffer[600];

	format_timespec(buffer, &spec);
	if (sprintf(buffer+strlen(buffer),",%d,%d,%d,%d,%d,%d,%d,%d,%d,%0.2f,%0.2f,%3.2f,%3.2f\n",
		mpu->rawMag[VEC3_X],
		mpu->rawMag[VEC3_Y],
		mpu->rawMag[VEC3_Z],
		mpu->rawAccel[VEC3_X],
		mpu->rawAccel[VEC3_Y],
		mpu->rawAccel[VEC3_Z],
		mpu->rawGyro[VEC3_X],
		mpu->rawGyro[VEC3_Y],
		mpu->rawGyro[VEC3_Z],
		mpu->pitch,
		mpu->roll,
		mpu->heading,
		mpu->ema_heading
		)) { 
				
		mm_append(buffer, mapped_file);
		file_idx = write_log_file("imu", runtime_count, file_idx, buffer);
	}
	return;
*/
}


void normalise3DVector(float *vector) {
	float R;
	R=sqrt( vector[0]*vector[0] + vector[1]*vector[1] +vector[2]*vector[2]);

	vector[0] /=R;
	vector[1] /=R;
	vector[2] /=R;
}

void read_loop(unsigned int sample_rate)
{
	if (sample_rate == 0)
		return;

	int i;
	int old_timestamp=0,T=0;
	float timestamp=0;
	mpudata_t mpu; memset(&mpu, 0, sizeof(mpudata_t));
	h_mmapped_file imu_mapped_file;
	struct timespec ts, old_ts, ts_end, spec;
	float process_time;

	float RwEst[] = {0,0,0};
	float RwAcc[] = {0,0,0};

	float Axz = 0;
	float Ayz = 0;

	float RwGyro[] = {0,0,0};

	char signRzGyro = 0;

	//int sample_id;

	//Prepare and initialise memory mapping
	memset(&imu_mapped_file, 0, sizeof(h_mmapped_file));

	imu_mapped_file.filename = "/sensors/imu";
	imu_mapped_file.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&imu_mapped_file)) < 0) {
		fprintf(stderr, "[IMU_daemon] ERROR mapping %s file.\n",imu_mapped_file.filename);
		return;	
	}

	runtime_count = read_rt_count();

	log_t sdLog = {"imu", "", runtime_count, 0, 0};
	if(creat_log(&sdLog) < 0) {
		return;
	}

	if (mpu9150_read(&mpu) == 0) {
		RwEst[VEC3_X] = mpu.calibratedAccel[VEC3_X]; 
		RwEst[VEC3_Y] = mpu.calibratedAccel[VEC3_Y]; 
		RwEst[VEC3_Z] = mpu.calibratedAccel[VEC3_Z];
	}

	mpu.ema_heading = 0.0;
	mpu.ema_alpha = 0.1;
		

	while (!done) {
		usleep(10000);

		if (mpu9150_read(&mpu) == 0) {
			old_ts=ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			timestamp = ts.tv_sec + ((float)ts.tv_nsec / NSEC_PER_MSEC)/1000;			
			spec = subtract_timespec(ts, old_ts);
			process_time = (spec.tv_sec * USEC_PER_SEC + spec.tv_nsec / NSEC_PER_USEC);

			time_s = ts.tv_sec;
			time_ms = ts.tv_nsec / NSEC_PER_MSEC;			
		
			acc[VEC3_X] = mpu.calibratedAccel[VEC3_X]; 
			acc[VEC3_Y] = mpu.calibratedAccel[VEC3_Y]; 
			acc[VEC3_Z] = -1*mpu.calibratedAccel[VEC3_Z];

			gyro_x_avg = (float)mpu.rawGyro[VEC3_X]/32768; 
			gyro_y_avg = (float)mpu.rawGyro[VEC3_Y]/32768; 
			gyro_z_avg = -1*(float)mpu.rawGyro[VEC3_Z]/32768; 

			gyro[VEC3_X]=(double)mpu.rawGyro[VEC3_X]*M_PI/180; //all in rad/s
			gyro[VEC3_Y]=(double)mpu.rawGyro[VEC3_Y]*M_PI/180;
			gyro[VEC3_Z]=-1*(double)mpu.rawGyro[VEC3_Z]*M_PI/180;
		
			normalise3DVector(acc);

			//**** Pitch/Roll Kalman filter
			if( RwEst[VEC3_Z] < 0.1 ) {
				RwGyro[VEC3_X] = RwEst[VEC3_X];
				RwGyro[VEC3_Y] = RwEst[VEC3_Y];
				RwGyro[VEC3_Z] = RwEst[VEC3_Z];
			}
			else {
				Axz = (atan2(RwEst[VEC3_X],RwEst[VEC3_Z]))
					+ (gyro_y_avg * 2000 * process_time/1000/1000);

				Ayz = (atan2(RwEst[VEC3_Y],RwEst[VEC3_Z]))
					+ (gyro_x_avg * 2000 * process_time/1000/1000);					

				signRzGyro = cos(Axz) >= 0 ? 1 : -1;

				RwGyro[VEC3_X] = sin(Axz)/sqrt( 1 + square(cos(Axz)) + square(tan(Ayz)) );  

				RwGyro[VEC3_Y] = sin(Ayz)/sqrt( 1 + square(cos(Ayz)) + square(tan(Axz)) );  
		
				RwGyro[VEC3_Z] = signRzGyro * sqrt( 1 - square(RwGyro[VEC3_X]) - square(RwGyro[VEC3_Y]) );  
			}

			RwEst[VEC3_X] = (acc[VEC3_X] + RwGyro[VEC3_X] * WGYRO) / (1 + WGYRO);
			RwEst[VEC3_Y] = (acc[VEC3_Y] + RwGyro[VEC3_Y] * WGYRO) / (1 + WGYRO);
			RwEst[VEC3_Z] = (acc[VEC3_Z] + RwGyro[VEC3_Z] * WGYRO) / (1 + WGYRO);

			normalise3DVector(RwEst);	

			mpu.kalman_pitch = (atan2(RwEst[VEC3_Y] , RwEst[VEC3_Z])*180/M_PI);
			mpu.kalman_roll = (atan2(RwEst[VEC3_X] , RwEst[VEC3_Z])*180/M_PI);

			mpu.pitch = (atan2(acc[VEC3_Y] , acc[VEC3_Z])*180/M_PI);
			mpu.roll = (atan2(acc[VEC3_X] , acc[VEC3_Z])*180/M_PI);	

			if(mpu.pitch < 0)
				mpu.pitch = mpu.pitch + 180;
			else
				mpu.pitch = mpu.pitch - 180;	

			if(mpu.roll < 0)
				mpu.roll += 180;
			else
				mpu.roll -= 180;
		

			//Heading - Pitch/Roll compensation 
			double Xh = (mpu.normMag[VEC3_X] * cos(mpu.roll*M_PI/180)) 
					- (mpu.normMag[VEC3_Z]*sin(mpu.roll*M_PI/180));
		  	double Yh = mpu.normMag[VEC3_X] * sin(mpu.pitch*M_PI/180)*sin(mpu.roll*M_PI/180) 
							+ mpu.normMag[VEC3_Y] *cos(mpu.pitch*M_PI/180) 
							- mpu.normMag[VEC3_Z] *sin(mpu.pitch*M_PI/180)*cos(mpu.roll*M_PI/180);
  
			mpu.heading = atan2(Yh, Xh);

			if (mpu.heading > M_PI)
			       mpu.heading = mpu.heading - 2*M_PI;
			else if (mpu.heading < -M_PI)
			       mpu.heading = mpu.heading + 2*M_PI;
			else if (mpu.heading < 0)
			       mpu.heading = mpu.heading + 2*M_PI;

			mpu.heading = mpu.heading* 180/M_PI;

			//Calculate expontential moving average.
			mpu.ema_heading = (mpu.ema_alpha*mpu.heading) + ( (1.0-mpu.ema_alpha)*mpu.ema_heading );

			//time_s[i] = ts.tv_sec;
			//time_ms[i] = ts.tv_nsec / NSEC_PER_MSEC;

			clock_gettime(CLOCK_REALTIME, &ts_end);

			tomappedMemXMLPacket(&mpu, &imu_mapped_file, ts_end, &sdLog);

			//tomappedMemXMLPacket_RAW(&mpu, &imu_mapped_file, ts_end, &sdLog);

			//sample_id=sample_id+1;
		}		
	}
}

void print_fused_euler_angles(mpudata_t *mpu)
{	//run : ./mnt/apps/mpu9150-cal/imu -b 0 -y 1
	printf("\rX: %0.2f Y: %0.2f Z: %0.2f        ",
			mpu->fusedEuler[VEC3_X] * RAD_TO_DEGREE, 
			mpu->fusedEuler[VEC3_Y] * RAD_TO_DEGREE, 
			mpu->fusedEuler[VEC3_Z] * RAD_TO_DEGREE);


	fflush(stdout);
}


char data_output[25];
void log_fused_euler_angles(mpudata_t *mpu)
{
	sprintf(data_output, "\rX: %0.1f Y: %0.1f Z: %0.1f        ",
			mpu->fusedEuler[VEC3_X] * RAD_TO_DEGREE, 
			mpu->fusedEuler[VEC3_Y] * RAD_TO_DEGREE, 
			mpu->fusedEuler[VEC3_Z] * RAD_TO_DEGREE);

	fwrite(data_output, 1, strlen(data_output), logfd);
	fflush(logfd);
}

void print_fused_quaternion(mpudata_t *mpu)
{
	printf("\rW: %0.2f X: %0.2f Y: %0.2f Z: %0.2f        ",
			mpu->fusedQuat[QUAT_W],
			mpu->fusedQuat[QUAT_X],
			mpu->fusedQuat[QUAT_Y],
			mpu->fusedQuat[QUAT_Z]);

	fflush(stdout);
}

void print_calibrated_accel(mpudata_t *mpu)
{
	printf("\rX: %f Y: %f Z: %f        ",
			mpu->calibratedAccel[VEC3_X], 
			mpu->calibratedAccel[VEC3_Y], 
			mpu->calibratedAccel[VEC3_Z]);

	fflush(stdout);
}

void print_calibrated_mag(mpudata_t *mpu)
{
	printf("\rX: %03d Y: %03d Z: %03d        ",
			mpu->calibratedMag[VEC3_X], 
			mpu->calibratedMag[VEC3_Y], 
			mpu->calibratedMag[VEC3_Z]);

	fflush(stdout);
}

int set_cal(int mag, char *cal_file)
{
	int i;
	FILE *f;
	char buff[32];
	float val[9];
	caldata_t cal;

	if (cal_file) {
		f = fopen(cal_file, "r");
		
		if (!f) {
			perror("open(<cal-file>)");
			return -1;
		}
	}
	else {
		if (mag) {
			f = fopen("./nand/magcal.txt", "r");
		
			if (!f) {
				fprintf(stderr, "\n[IMU_daemon] ERROR Default magcal.txt not found\n");
				return 0;
			}
		}
		else {
			f = fopen("./nand/accelcal.txt", "r");
		
			if (!f) {
				fprintf(stderr, "\n[IMU_daemon] ERROR Default accelcal.txt not found\n");
				return 0;
			}
		}		
	}

	memset(buff, 0, sizeof(buff));
	
	for (i = 0; i < 12; i++) {
		if (!fgets(buff, 20, f)) {
			fprintf(stderr, "\n[IMU_daemon] ERROR Not enough lines in calibration file\n");
			break;
		}

		val[i] = atof(buff);
	}

	fclose(f);

	if (i != 12) 
		return -1;

	if (mag) {
		for (i = 0; i < 9; i++)
			cal.comp[i] = val[i];

		cal.offset[0] = val[9];
		cal.offset[1] = val[10];
		cal.offset[2] = val[11];

		mpu9150_set_mag_cal(&cal);
	}
	else {
		cal.offset[0] = val[0];//-168.1237;
		cal.offset[1] = val[1];//211.8347;
		cal.offset[2] = val[2];//885.873;

		cal.range[0] = val[3];//16531.6599;
		cal.range[1] = val[4];//16453.0189;
		cal.range[2] = val[5];//16418.2263;

		mpu9150_set_accel_cal(&cal);
	}

	return 0;
}

void set_config(double *val) {
/*	FILE *f;
	char buff[32];
	double val[1];
	int i;

	f = fopen("./nand/magcal.txt", "r");
		
	if (!f) {
		printf("Default imu_config.txt not found\n");
		return 0;
	}

	memset(buff, 0, sizeof(buff));
	
	for (i = 0; i < 1; i++) {
		if (!fgets(buff, 20, f)) {
			printf("Not enough lines in calibration file\n");
			break;
		}

		val[i] = atof(buff);
	}
	*/
	//mpu9150_set_config(&val);
}

void register_sig_handler()
{
	struct sigaction sia;

	bzero(&sia, sizeof sia);
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		exit(1);
	} 
}

void sigint_handler(int sig)
{
	done = 1;
}
