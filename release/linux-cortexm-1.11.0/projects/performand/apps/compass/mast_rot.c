#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "i2c-dev.h"

#define SCALE 2  // accel full-scale, should be 2, 4, or 8
 
/* LSM303 Address definitions */
#define LSM303_MAG  0x1E  // assuming SA0 grounded
#define LSM303_ACC  0x18  // assuming SA0 grounded
 
#define X 0
#define Y 1
#define Z 2
 
/* LSM303 Register definitions */
#define CTRL_REG1_A 0x20
#define CTRL_REG2_A 0x21
#define CTRL_REG3_A 0x22
#define CTRL_REG4_A 0x23
#define CTRL_REG5_A 0x24
#define HP_FILTER_RESET_A 0x25
#define REFERENCE_A 0x26
#define STATUS_REG_A 0x27
#define OUT_X_L_A 0x28
#define OUT_X_H_A 0x29
#define OUT_Y_L_A 0x2A
#define OUT_Y_H_A 0x2B
#define OUT_Z_L_A 0x2C
#define OUT_Z_H_A 0x2D
#define INT1_CFG_A 0x30
#define INT1_SOURCE_A 0x31
#define INT1_THS_A 0x32
#define INT1_DURATION_A 0x33
#define CRA_REG_M 0x00
#define CRB_REG_M 0x01
#define MR_REG_M 0x02
#define OUT_X_H_M 0x03
#define OUT_X_L_M 0x04
#define OUT_Y_H_M 0x05
#define OUT_Y_L_M 0x06
#define OUT_Z_H_M 0x07
#define OUT_Z_L_M 0x08
#define SR_REG_M 0x09
#define IRA_REG_M 0x0A
#define IRB_REG_M 0x0B
#define IRC_REG_M 0x0C

#define PI 3.14159265
//#define M_PI 3.14159265358979323846

int i2cset(int i2cbus, int address, int daddress, int value)
{
        char *end;
        int res, size, file;
        char filename[20];

        size = I2C_SMBUS_BYTE;

        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
        if (file<0) {
                if (errno == ENOENT) {
                        fprintf(stderr, "Error: Could not open file "
                                "/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
                } else {
                        fprintf(stderr, "Error: Could not open file "
                                "`%s': %s\n", filename, strerror(errno));
                        if (errno == EACCES)
                                fprintf(stderr, "Run as root?\n");
                }
                exit(1);
        }

        if (ioctl(file, I2C_SLAVE, address) < 0) {
                fprintf(stderr,
                        "Error: Could not set address to 0x%02x: %s\n",
                        address, strerror(errno));
                return -errno;
        }

        res = i2c_smbus_write_byte_data(file, daddress, value);
        if (res < 0) {
                fprintf(stderr, "Warning - write failed, filename=%s, daddress=%d\n",
                        filename, daddress);
        }
        
        return 0;
}


int i2cget(int i2cbus, int address, int daddress)
{
        char *end;
        int res, size, file;
        
        char filename[20];

        size = I2C_SMBUS_BYTE;


        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
        if (file<0) {
                if (errno == ENOENT) {
                        fprintf(stderr, "Error: Could not open file "
                                "/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
                } else {
                        fprintf(stderr, "Error: Could not open file "
                                "`%s': %s\n", filename, strerror(errno));
                        if (errno == EACCES)
                                fprintf(stderr, "Run as root?\n");
                }
                exit(1);
        }

        if (ioctl(file, I2C_SLAVE, address) < 0) {
                fprintf(stderr,
                        "Error: Could not set address to 0x%02x: %s\n",
                        address, strerror(errno));
                return -errno;
        }

        res = i2c_smbus_write_byte(file, daddress);
        if (res < 0) {
                fprintf(stderr, "Warning - write failed, filename=%s, daddress=%d\n",
                        filename, daddress);
        }
        
        res = i2c_smbus_read_byte_data(file, daddress);
        close(file);

        if (res < 0) {
                fprintf(stderr, "Error: Read failed, res=%d\n", res);
                exit(2);
        }

        return res;
}

void write_log(char *receiveMessage) { 
	FILE *file;
	int size=0;
	long usedBytes;

	//if(statfs("/dev/mmcblk0p1", &_statfs)<0)
	//	printf("Error can't determine remain capacity on SD-Card\n");

	//usedBytes=(_statfs.f_blocks-_statfs.f_bfree)*_statfs.f_bsize;	 
	//printf("SD Card Capacity: %d\n",_statfs.f_blocks);

	char format[] = "/mnt/testlog-%s" ".txt";
	char filename[sizeof format+10];
	sprintf(filename,format, "compass");	

	file = fopen(filename,"a+"); 	
	fwrite(receiveMessage,1,strlen(receiveMessage),file);
	fclose(file);

}


int main(int argc, char *argv[])
{
        int i2cbus, address, daddress;
        int i = 0;
	short xhigh, xlow, yhigh, ylow, zhigh, zlow;
	short acc_raw[3], acc_min[3], acc_max[3];
	short magValue[3], mag_min[3], mag_max[3];
	float acc[3], head[3], roll, pitch, mag;
	double heading = 0.0;
	double headingDegrees = 0.0;
	char buffer[100];
	

        i2cbus = 0;

	// Initialize the LSM303, using a SCALE full-scale range
	i2cset(i2cbus, LSM303_ACC,CTRL_REG1_A, 0x27); // 0x27 = normal power mode, all accel axes on

	if ((SCALE==8)||(SCALE==4))
		i2cset(i2cbus, LSM303_ACC, CTRL_REG4_A, (0x00 | (SCALE-SCALE/2-1)<<4));
	 else
		i2cset(i2cbus, LSM303_ACC, CTRL_REG4_A, 0x00); 
	
	i2cset(i2cbus, LSM303_MAG, CRA_REG_M, 0x14);	// 0x14 = mag 30Hz output rate
	i2cset(i2cbus, LSM303_MAG, MR_REG_M, 0x00);	// 0x00 = continouous conversion mode	


        while(1){
		usleep(50000);
		acc_raw[X]   = ((i2cget(i2cbus, LSM303_ACC, OUT_X_H_A) << 8) | i2cget(i2cbus, LSM303_ACC, OUT_X_L_A));
		acc_raw[Y]   = ((i2cget(i2cbus, LSM303_ACC, OUT_Y_H_A) << 8) | i2cget(i2cbus, LSM303_ACC, OUT_Y_L_A));
		acc_raw[Z]   = ((i2cget(i2cbus, LSM303_ACC, OUT_Z_H_A) << 8) | i2cget(i2cbus, LSM303_ACC, OUT_Z_L_A));

		/*xhigh = i2cget(i2cbus, LSM303_MAG, OUT_X_H_M);
		xlow = i2cget(i2cbus, LSM303_MAG, OUT_X_L_M);
		yhigh = i2cget(i2cbus, LSM303_MAG, OUT_Y_H_M);
		ylow = i2cget(i2cbus, LSM303_MAG, OUT_Y_L_M);
		zhigh = i2cget(i2cbus, LSM303_MAG, OUT_Z_H_M);
		zlow = i2cget(i2cbus, LSM303_MAG, OUT_Z_L_M);__*/

		//printf("x=%02d %02d y=%02d %02d z=%02d %02d\r\n",xhigh,xlow,yhigh,ylow,zhigh,zlow);

		magValue[X] = ((i2cget(i2cbus, LSM303_MAG, OUT_X_H_M) << 8) | i2cget(i2cbus, LSM303_MAG, OUT_X_L_M)); 
		magValue[Z] = ((i2cget(i2cbus, LSM303_MAG, OUT_Y_H_M) << 8) | i2cget(i2cbus, LSM303_MAG, OUT_Y_L_M)); 		
		magValue[Y] = ((i2cget(i2cbus, LSM303_MAG, OUT_Z_H_M) << 8) | i2cget(i2cbus, LSM303_MAG, OUT_Z_L_M)); 

/*		for(i=0;i<3;i++) {		
			if (acc_raw[i] < acc_min[i]) acc_min[i] = acc_raw[i];
			if (acc_raw[i] > acc_max[i]) acc_max[i] = acc_raw[i];
			if (magValue[i] < mag_min[i]) mag_min[i] = magValue[i];
			if (magValue[i] > mag_max[i]) mag_max[i] = magValue[i];
		}
*///		for(i=0;i<3;i++)
//			printf("%5d (%5d %5d) | ",magValue[i], mag_min[i],mag_max[i]);
//		printf("\r\n");

		printf("%5d %5d %5d \r\n",magValue[X],magValue[Y],magValue[Z]);
		sprintf(buffer, "%d\t%d\t%d \r\n",magValue[X],magValue[Y],magValue[Z]);
		write_log(buffer);
		
		//for (i=0; i<3; i++)
    		//	acc[i] = acc_raw[i] / pow(2, 15) * SCALE;

		//Roll & Pitch Equations ...
    		/*pitch  = (atan2(acc[Y], acc[Z])*180.0)/M_PI;
    		roll = (atan2(acc[X], sqrt(acc[Y]*acc[Y] + acc[Z]*acc[Z]))*180.0)/M_PI;
		
		//Calculate heading ...
		head[X] = magValue[X] * cos(pitch) + magValue[Z] * sin(pitch);
  		head[Y] = magValue[X] * sin(roll) * sin(pitch) + magValue[Y] * cos(roll) - magValue[Z] * sin(roll) * cos(pitch);
  		head[Z] = -magValue[X] * cos(roll) * sin(pitch) + magValue[Y] * sin(roll) + magValue[Z] * cos(roll) * cos(pitch);
 
		mag = sqrt(head[X]*head[X] + head[Y]*head[Y] + head[Z]*head[Z]);

 		//heading = 180 * atan(head[X]/head[Y])/PI;
  		heading = 180*atan2(magValue[Y], magValue[X])/PI;  // assume pitch, roll are 0
		
  		//if (heading <0)
    		//	heading += 360;

		if (head[Y] < 0)
    			heading = (360 + heading);

		printf("x = %5d\t y = %5d\t z = %5df\t mag = %3.2f roll = %2.1f pitch = %2.1f heading = %3.1f\r\n", 
				magValue[X], 
				magValue[Y], 
				magValue[Z],
				mag,
				roll,
				pitch,
				heading); */
		

		

//getLSM303_accel(accel);  // get the acceleration values and store them in the accel array
//  while(!(LSM303_read(SR_REG_M) & 0x01))
//    ;  // wait for the magnetometer readings to be ready
//  getLSM303_mag(mag);  // get the magnetometer values, store them in mag


                

                //small wait
                //usleep(50000);                        

                //z_val = ((i2cget(i2cbus, address, 5) << 8) + i2cget(i2cbus, address, 6));

                //small wait
                //usleep(50000);        

                //y = ((i2cget(i2cbus, address, 7) << 8) + i2cget(i2cbus, address, 8));

		// Calculate heading when the magnetometer is level, then correct for signs of axis.
		//heading = atan2(y, x); //2*atan(y/(sqrtf(x^2+y^2)+x)); //;

		// Correct for when signs are reversed.
		//if(heading < 0)
		//	heading += 2*PI;

		// Convert radians to degrees for readability.
		//headingDegrees = heading * 180/M_PI; 

                // 
                
                //small wait
                               

        }        
        
        return 0;

}
