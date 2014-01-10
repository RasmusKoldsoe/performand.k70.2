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

#include "mpu9150.h"
#include "linux_glue.h"
#include "local_defaults.h"

#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/utils.h"

int set_cal(int mag, char *cal_file);
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

float x_avg=0,y_avg=0,z_avg=0;
int acc_x[5];
int acc_y[5];	
int acc_z[5];
int time_s[5];
int time_ms[5];

void usage(char *argv_0)
{
	printf("\nUsage: %s [options]\n", argv_0);
	printf("  -b <i2c-bus>          The I2C bus number where the IMU is. The default is 1 to use /dev/i2c-1.\n");
	printf("  -s <sample-rate>      The IMU sample rate in Hz. Range 2-50, default 10.\n");
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
	int sample_rate = 50; //DEFAULT_SAMPLE_RATE_HZ;
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
		printf("IMU Error: Initialisation failed.\n");
		return 0;
	}

	set_cal(0, accel_cal_file);
	set_cal(1, mag_cal_file);

	if (accel_cal_file)
		free(accel_cal_file);

	if (mag_cal_file)
		free(mag_cal_file);
 
	//logfd = fopen(log_file_name, "w");
	//if( NULL == logfd ) 
	//	mpu9150_exit();

	read_loop(sample_rate);

	//fclose(logfd);

	mpu9150_exit();

	return 0;
}

void tomappedMemXMLPacket(mpudata_t *mpu, h_mmapped_file *mapped_file, int *sample_id, struct timespec spec) {
	char buffer[600];

	format_timespec(buffer, &spec);
	if (sprintf(buffer+strlen(buffer),",$IMU,%d,%0.2f,%0.2f,%0.2f,%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%d,%d,%d,%d.%d,%d,%d,%d,%d.%d\n",
/*"\n<imu id=\"%d\">\n\
<Timestamp>%d.%d</Timestamp>\n\
<Orientation>\n\
<pitch>%0.2f</pitch>\n\
<roll>%0.2f</roll>\n\
<yaw>%0.2f</yaw>\n\
</Orientation>\n\
<Accelerometer>\n\
<x id=\"1\">%05d</x>\n\
<y id=\"1\">%05d</y>\n\
<z id=\"1\">%05d</z>\n\
<x id=\"2\">%05d</x>\n\
<y id=\"2\">%05d</y>\n\
<z id=\"2\">%05d</z>\n\
<time id=\"2\">%d.%d</time>\n\
<x id=\"3\">%05d</x>\n\
<y id=\"3\">%05d</y>\n\
<z id=\"3\">%05d</z>\n\
<time id=\"3\">%d.%d</time>\n\
<x id=\"4\">%05d</x>\n\
<y id=\"4\">%05d</y>\n\
<z id=\"4\">%05d</z>\n\
<time id=\"4\">%d.%d</time>\n\
<x id=\"5\">%05d</x>\n\
<y id=\"5\">%05d</y>\n\
<z id=\"5\">%05d</z>\n\
<time id=\"5\">%d.%d</time>\n\
</Accelerometer>\n\
</imu>\n",*/
		*sample_id,
		x_avg,
		y_avg,
		z_avg,		
		acc_x[0],
		acc_y[0],
		acc_z[0],
		time_s[0],time_ms[0],
		acc_x[1],
		acc_y[1],
		acc_z[1],
		time_s[1],time_ms[1],
		acc_x[2],
		acc_y[2],
		acc_z[2],
		time_s[2],time_ms[2],
		acc_x[3],
		acc_y[3],
		acc_z[3],
		time_s[3],time_ms[3],
		acc_x[4],
		acc_y[4],
		acc_z[4],
		time_s[4],time_ms[4]) >=0 ) { 
				
		mm_append(buffer, mapped_file);
		file_idx = write_log_file("imu", runtime_count, file_idx, buffer);
	}
	return;
}

void read_loop(unsigned int sample_rate)
{
	if (sample_rate == 0)
		return;

	int i;
	mpudata_t mpu; memset(&mpu, 0, sizeof(mpudata_t));
	h_mmapped_file imu_mapped_file;
	struct timespec ts_begin, ts_end, ts_interval;
	int sample_id;

	//Prepare and initialise memory mapping
	memset(&imu_mapped_file, 0, sizeof(h_mmapped_file));

	imu_mapped_file.filename = "imu";
	imu_mapped_file.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&imu_mapped_file)) < 0) {
		printf("Error mapping %s file.\n",imu_mapped_file.filename);
		return;	
	}

	runtime_count = read_rt_count();

	while (!done) {
		/*if (mpu9150_read(&mpu) == 0) {
			print_fused_euler_angles(&mpu);
			//log_fused_euler_angles(&mpu);
			// print_fused_quaternion(&mpu);
			// print_calibrated_accel(&mpu);
			// print_calibrated_mag(&mpu);
		}

		linux_delay_ms(loop_delay); */

		x_avg = 0;
		y_avg = 0;
		z_avg = 0;

		for(i=0;i<5;i++) {
			usleep(10000);

			if (mpu9150_read(&mpu) == 0) {
				clock_gettime(CLOCK_REALTIME, &ts_begin);

				x_avg += mpu.fusedEuler[VEC3_X] * RAD_TO_DEGREE;
				y_avg += mpu.fusedEuler[VEC3_Y] * RAD_TO_DEGREE;
				z_avg += mpu.fusedEuler[VEC3_Z] * RAD_TO_DEGREE;

				acc_x[i] = mpu.calibratedAccel[VEC3_X]; 
				acc_y[i] = mpu.calibratedAccel[VEC3_Y]; 
				acc_z[i] = mpu.calibratedAccel[VEC3_Z];

				time_s[i] = ts_begin.tv_sec;
				time_ms[i] = ts_begin.tv_nsec / NSEC_PER_MSEC;

				/*if (mpu9150_read(&mpu) == 0) {
					printf("\rX: %05d Y: %05d Z: %05d        \n",
						mpu.calibratedAccel[VEC3_X], 
						mpu.calibratedAccel[VEC3_Y], 
						mpu.calibratedAccel[VEC3_Z]);
				}
				fflush(stdout);*/
				clock_gettime(CLOCK_REALTIME, &ts_end);
			}
		}

		x_avg /= i;
		y_avg /= i;
		z_avg /= i;

		//printf("\rX: %0.2f Y: %0.2f Z: %0.2f        \n",x_avg , y_avg , z_avg);
		/*if (mpu9150_read(&mpu) == 0) {
		printf("\rX: %05d Y: %05d Z: %05d        \n",
			mpu.calibratedAccel[VEC3_X], 
			mpu.calibratedAccel[VEC3_Y], 
			mpu.calibratedAccel[VEC3_Z]);
		}*/
		//fflush(stdout);

		tomappedMemXMLPacket(&mpu, &imu_mapped_file, &sample_id, ts_end);
		
		sample_id=sample_id+1;
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
	printf("\rX: %05d Y: %05d Z: %05d        ",
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
	long val[6];
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
				printf("Default magcal.txt not found\n");
				return 0;
			}
		}
		else {
			f = fopen("./nand/accelcal.txt", "r");
		
			if (!f) {
				printf("Default accelcal.txt not found\n");
				return 0;
			}
		}		
	}

	memset(buff, 0, sizeof(buff));
	
	for (i = 0; i < 6; i++) {
		if (!fgets(buff, 20, f)) {
			printf("Not enough lines in calibration file\n");
			break;
		}

		val[i] = atoi(buff);

		if (val[i] == 0) {
			printf("Invalid cal value: %s\n", buff);
			break;
		}
	}

	fclose(f);

	if (i != 6) 
		return -1;

	cal.offset[0] = (short)((val[0] + val[1]) / 2);
	cal.offset[1] = (short)((val[2] + val[3]) / 2);
	cal.offset[2] = (short)((val[4] + val[5]) / 2);

	cal.range[0] = (short)(val[1] - cal.offset[0]);
	cal.range[1] = (short)(val[3] - cal.offset[1]);
	cal.range[2] = (short)(val[5] - cal.offset[2]);
	
	if (mag) 
		mpu9150_set_mag_cal(&cal);
	else 
		mpu9150_set_accel_cal(&cal);

	return 0;
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
