#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "i2c-dev.h"

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


int main(int argc, char *argv[])
{
        int i2cbus, address, daddress;
        short x, y, z_val;
	double heading = 0.0;
	double headingDegrees = 0.0;

        i2cbus = 0;
        address = 0x1E;

        //continuously read and print the values for each axis
        while(1){
                //set to continuous measurement mode
                i2cset(i2cbus, address, 2, 0);

                x = ((i2cget(i2cbus, address, 3) << 8) + i2cget(i2cbus, address, 4));

                //small wait
                usleep(50000);                        

                z_val = ((i2cget(i2cbus, address, 5) << 8) + i2cget(i2cbus, address, 6));

                //small wait
                usleep(50000);        

                y = ((i2cget(i2cbus, address, 7) << 8) + i2cget(i2cbus, address, 8));

		// Calculate heading when the magnetometer is level, then correct for signs of axis.
		heading = atan2(y, x); //2*atan(y/(sqrtf(x^2+y^2)+x)); //;

		// Correct for when signs are reversed.
		if(heading < 0)
			heading += 2*PI;

		// Convert radians to degrees for readability.
		headingDegrees = heading * 180/M_PI; 

               printf("x = %d, y = %d, heading = %3.2f\r\n", x, y, headingDegrees);
                
                //small wait
                usleep(50000);                

        }        
        
        return 0;

}
