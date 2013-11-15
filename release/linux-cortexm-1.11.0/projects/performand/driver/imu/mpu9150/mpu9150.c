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

#include "mpu9150.h"

static int data_ready(void);
static void calibrate_data(mpudata_t *mpu);
//static void tilt_compensate(quaternion_t magQ, quaternion_t unfusedQ);
//static int data_fusion(mpudata_t *mpu);
//static unsigned short inv_row_2_scale(const signed char *row);
static unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx);

int debug_on;
int yaw_mixing_factor;

int use_accel_cal;
caldata_t accel_cal_data;

int use_mag_cal;
caldata_t mag_cal_data;

static char status_string[200];

int runtime_count = 0;

struct int_param_s init_param = {
	.pin = 0
};

/*void mpu9150_print_int_status(void) {
	short status;
	if(mpu_get_int_status(&status))
		printk("Failed read int status\n");
	printk("status: 0x%x\n",status);
}*/

int mpu9150_print_data(mpudata_t *mpu, char *string)
{
/*	short rawGyro[3];
	short rawAccel[3];
	long rawQuat[4]; // w, x, y, z
	unsigned long dmpTimestamp;

	short rawMag[3];
	unsigned long magTimestamp;
 */

	char quat_string[100], dmp_time_string[20], rawMag_string[100], rawAcc_string[100];
	if(!mpu) {
		//sprintf(quat_string, "-");
		//sprintf(dmp_time_string, "-");
		//sprintf(rawMag_string, "-");
		//sprintf(rawAcc_string, "-");	
		sprintf(string, "\n<imu id=\"%d\">\n<Status>\n%s</Status>\n</imu>\n",runtime_count,status_string);
	}
	else {
		sprintf(quat_string, "<W>%lu</W>\n<X>%lu</X>\n<Y>%lu</Y>\n<Z>%lu</Z>\n", mpu->rawQuat[0], mpu->rawQuat[1], mpu->rawQuat[2], mpu->rawQuat[3]);
		sprintf(dmp_time_string, "%lu", mpu->dmpTimestamp);
		sprintf(rawMag_string, "<X>%d</X>\n<Y>%d</Y>\n<Z>%d</Z>\n<Timestamp>%lu</Timestamp>\n", \
					(int)mpu->rawMag[0], (int)mpu->rawMag[1], (int)mpu->rawMag[2], mpu->magTimestamp);
		sprintf(rawAcc_string, "<X>%d</X>\n<Y>%d</Y>\n<Z>%d</Z>\n<Timestamp>%lu</Timestamp>\n", \
					(int)mpu->rawAccel[0], (int)mpu->rawAccel[1], (int)mpu->rawAccel[2], mpu->magTimestamp);

		/*sprintf(string, "%d, %d, %d, %d, %d, %d, %lu, %lu, %lu, %lu, %lu, %d, %d, %d, %lu", \
			(int)mpu->rawGyro[0], (int)mpu->rawGyro[1], (int)mpu->rawGyro[2], \
			(int)mpu->rawAccel[0], (int)mpu->rawAccel[1], (int)mpu->rawAccel[2], \
			mpu->rawQuat[0], mpu->rawQuat[1], mpu->rawQuat[2], mpu->rawQuat[3], \
			mpu->dmpTimestamp, \
			(int)mpu->rawMag[0], (int)mpu->rawMag[1], (int)mpu->rawMag[2], \
			mpu->magTimestamp);*/
		sprintf(string, "\n<imu id=\"%d\">\n\
<Quaternion>\n%s\
<Timestamp>%s</Timestamp>\n\
</Quaternion>\n\
<Magnetometer>\n%s</Magnetometer>\n\
<Accelerometer>\n%s</Accelerometer>\n\
</imu>\n",runtime_count, quat_string, dmp_time_string, rawMag_string, rawAcc_string, status_string);
//<Status>\n%s</Status>\n", quat_string, dmp_time_string, rawMag_string, rawAcc_string, status_string);

	} 
	runtime_count+=1;
	
	return 0;
}

void mpu9150_set_debug(int on)
{
	printk("Set debug: %s\n", on?"true":"false");
	debug_on = on;
}

int __init mpu9150_init(int sample_rate, int mix_factor)
{
	signed char gyro_orientation[9] = { 1, 0, 0,
                                            0, 1, 0,
                                            0, 0, 1 };

	if (sample_rate < MIN_SAMPLE_RATE || sample_rate > MAX_SAMPLE_RATE) {
		printk(KERN_WARNING "Invalid sample rate %d\n", sample_rate);
		sprintf(status_string,"Invalid sample rate %d\n", sample_rate);
		return -1;
	}

	if (mix_factor < 0 || mix_factor > 100) {
		printk(KERN_WARNING "Invalid mag mixing factor %d\n", mix_factor);
		return -1;
	}

	yaw_mixing_factor = mix_factor;

	printk("\nInitializing IMU ...");

	if (mmpu_init()) {
		printk(KERN_WARNING "\nmpu_init() failed\n");
		sprintf(status_string,"mpu_init() failed\n");
		return -1;
	}

	//printk(".");

	if (mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS)) {
		printk(KERN_WARNING "\nmpu_set_sensors() failed\n");
		sprintf(status_string,"mpu_set_sensors() failed\n");
		return -1;
	}

	//printk(".");

	if (mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL)) {
		printk(KERN_WARNING "\nmpu_configure_fifo() failed\n");
		sprintf(status_string,"mpu_configure_fifo() failed\n");
		return -1;
	}

	//printk(".");
	
	if (mpu_set_sample_rate(sample_rate)) {
		printk(KERN_WARNING "\nmpu_set_sample_rate() failed\n");
		sprintf(status_string,"mpu_set_sample_rate() failed\n");
		return -1;
	}

	//printk(".");

	if (mpu_set_compass_sample_rate(sample_rate)) {
		printk(KERN_WARNING "\nmpu_set_compass_sample_rate() failed\n");
		return -1;
	}

	//printk(".");

	if (dmp_load_motion_driver_firmware()) {
		printk(KERN_WARNING "\ndmp_load_motion_driver_firmware() failed\n");
		return -1;
	}

	//printk(".");

	if (dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation))) {
		printk(KERN_WARNING "\ndmp_set_orientation() failed\n");
		return -1;
	}

	//printk(".");

  	if (dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_SEND_RAW_ACCEL 
						| DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL)) {
		printk(KERN_WARNING "\ndmp_enable_feature() failed\n");
		return -1;
	}

	//printk(".");
 
	if (dmp_set_fifo_rate(sample_rate)) {
		printk(KERN_WARNING "\ndmp_set_fifo_rate() failed\n");
		return -1;
	}

	//printk(".");

	if (mpu_set_dmp_state(1)) {
		printk(KERN_WARNING "\nmpu_set_dmp_state(1) failed\n");
		return -1;
	}

	printk(" done\n");

	return 0;
}

void __exit mpu9150_exit()
{
	// turn off the DMP on exit 
	if (mpu_set_dmp_state(0))
		printk(KERN_WARNING"mpu_set_dmp_state(0) failed\n");

	// TODO: Should turn off the sensors too
}

int mpu9150_suspend(void)
{
	return mpu_suspend();
}

int mpu9150_resume(void)
{
	return mpu_resume();
}

void mpu9150_set_accel_cal(caldata_t *cal)
{
	int i;
	long bias[3];

	if (!cal) {
		use_accel_cal = 0;
		return;
	}

	memcpy(&accel_cal_data, cal, sizeof(caldata_t));

	for (i = 0; i < 3; i++) {
		if (accel_cal_data.range[i] < 1)
			accel_cal_data.range[i] = 1;
		else if (accel_cal_data.range[i] > ACCEL_SENSOR_RANGE)
			accel_cal_data.range[i] = ACCEL_SENSOR_RANGE;

		bias[i] = -accel_cal_data.offset[i];
	}

	if (debug_on) {
		printk(KERN_DEBUG "\naccel cal (range : offset)\n");

		for (i = 0; i < 3; i++)
			printk(KERN_DEBUG "%d : %d\n", accel_cal_data.range[i], accel_cal_data.offset[i]);
	}

	mpu_set_accel_bias(bias);

	use_accel_cal = 1;
}

void mpu9150_set_mag_cal(caldata_t *cal)
{
	int i;

	if (!cal) {
		use_mag_cal = 0;
		return;
	}

	memcpy(&mag_cal_data, cal, sizeof(caldata_t));

	for (i = 0; i < 3; i++) {
		if (mag_cal_data.range[i] < 1)
			mag_cal_data.range[i] = 1;
		else if (mag_cal_data.range[i] > MAG_SENSOR_RANGE)
			mag_cal_data.range[i] = MAG_SENSOR_RANGE;

		if (mag_cal_data.offset[i] < -MAG_SENSOR_RANGE)
			mag_cal_data.offset[i] = -MAG_SENSOR_RANGE;
		else if (mag_cal_data.offset[i] > MAG_SENSOR_RANGE)
			mag_cal_data.offset[i] = MAG_SENSOR_RANGE;
	}

	if (debug_on) {
		printk(KERN_DEBUG "\nmag cal (rangedmp_read_fifo() failed\n : offset)\n");

		for (i = 0; i < 3; i++)
			printk(KERN_DEBUG "%d : %d\n", mag_cal_data.range[i], mag_cal_data.offset[i]);
	}

	use_mag_cal = 1;
}

int mpu9150_read_dmp(mpudata_t *mpu)
{
	short sensors;
	unsigned char more;

	if (data_ready() == 0)
		return -1;


	if (dmp_read_fifo(mpu->rawGyro, mpu->rawAccel, mpu->rawQuat, &mpu->dmpTimestamp, &sensors, &more) < 0) {
		printk(KERN_WARNING "dmp_read_fifo() failed\n");
		sprintf(status_string,"dmp_read_fifo() failed\n");
		return -1;
	}

	while (more) {
		// Fell behind, reading again
		if (dmp_read_fifo(mpu->rawGyro, mpu->rawAccel, mpu->rawQuat, &mpu->dmpTimestamp, &sensors, &more) < 0) {
			printk(KERN_WARNING "dmp_read_fifo() failed\n");
			sprintf(status_string,"dmp_read_fifo() failed\n");
			return -1;
		}
	}

	return 0;
}

int mpu9150_read_mag(mpudata_t *mpu)
{
	int error_code = 0;
	if ((error_code = mpu_get_compass_reg(mpu->rawMag, &mpu->magTimestamp)) < 0) {
		printk(KERN_WARNING "mpu_get_compass_reg() failed. Error %d\n", error_code);
		sprintf(status_string,"dmp_read_fifo() failed\n");
		return -1;
	}

	return 0;
}

int mpu9150_read(mpudata_t *mpu)
{
//printk("mpu9150_read...");
int _start = jiffies;
	if (mpu9150_read_dmp(mpu) != 0)
		return -1;

	if (mpu9150_read_mag(mpu) != 0)
		return -1;

	//calibrate_data(mpu);

	//printk("Time for read mpu: %d\n", jiffies_to_msecs(jiffies-_start));
	return 0; //data_fusion(mpu);
}

int data_ready(void)
{
	short status;

	if (mpu_get_int_status(&status) < 0) {
		printk(KERN_WARNING "mpu_get_int_status() failed\n");
		return 0;
	}

	return (status & (MPU_INT_STATUS_DATA_READY | MPU_INT_STATUS_DMP | MPU_INT_STATUS_DMP_0));
}


/* These next two functions convert the orientation matrix (see
 * gyro_orientation) to a scalar representation for use by the DMP.
 * NOTE: These functions are borrowed from InvenSense's MPL.
 */
unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;      // error
    return b;
}

unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    /*
       XYZ  010_001_000 Identity Matrix
       XZY  001_010_000
       YXZ  010_000_001
       YZX  000_010_001
       ZXY  001_000_010
       ZYX  000_001_010
     */
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

