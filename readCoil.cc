#include <stdio.h>
#include <stdlib.h>
// #include <stdarg.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <dirent.h>
using namespace std;

#include <H5Cpp.h>
using namespace H5;

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "spidriver_host.h"
#include "adcdriver_host.h"

#define SPI_PRU 0
#define DRV_PRU 1

#define CLK_GPIO 66
#define STATUS_LED 87

#define SPIBUFFERSIZE 1000

#define RAMOFFSET 0x80
static uint32_t *pru1_dataram;

//const string FILE_PATH("/var/lib/cloud9/LogSNIPE/SNIPE_Data/");
const string FILE_PATH("/home/debian/sdcard/SNIPE_Data/");
const string FILE_PREFIX("Lew_");
string filename;
const string TIMESET_NAME("TimeArray");
const string VOLTSET_NAME("VoltArray");

double timeout = 40;

int readGPIO(int GPIO)
{
    char filename[32];
    FILE *fp;
    int v;
    
    snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/value",GPIO);
    fp = fopen(filename,"r");
    fscanf(fp,"%d",(int *)&v);
    fclose(fp);
    return (v);
}

void writeGPIO(int GPIO, int v)
{
    char filename[32];
    FILE *fp;
    
    snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/value",GPIO);
    fp = fopen(filename,"w");
    fprintf(fp,"%d\n",(int) v);
    fclose(fp);
}


// return the epoch time as a double to the nearest nano second
double epoch_double(struct timespec *tv) {
    if(clock_gettime(CLOCK_REALTIME, tv))
        perror("error clock_gettime\n");
      
    char time_str[32];
    
    sprintf(time_str, "%ld.%.9ld", tv->tv_sec, tv->tv_nsec);
    
    return atof(time_str);
} // epoch_double

int main(void) {
    
    if(system("systemctl restart ntp") == -1){
        exit(-1);
    }
    if(system("systemctl restart gpsd") == -1){
        exit(-1);
    }
    if(system("echo 66 > /sys/class/gpio/export") == -1){
        exit(-1);
    }
    if(system("echo 87 > /sys/class/gpio/export") == -1){
        exit(-1);
    }
    if(system("mount /dev/mmcblk0p1 /home/debian/sdcard/") == -1){
        exit(-1);
    }
    
    // Time vars
    struct timespec tv;
    struct timespec *ptv = &tv;
    
    // PRU stuf
    prussdrv_open(PRU_EVTOUT_1);
    prussdrv_pru_reset(DRV_PRU);
    prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, (void **) &pru1_dataram);
    
    // Data buffers
    uint32_t *pbuf1, *pbuf2;
    const uint32_t ram_offset = RAMOFFSET;
    pbuf1 = pru1_dataram + ram_offset;
    pbuf2 = pru1_dataram + ram_offset + 1;
    int buf1, buf2;
    int wrote1, wrote2;
    
    // Write buffer
    uint32_t writeBuffer[SPIBUFFERSIZE];
    
    // Data structure
    int timeArrayLen = 1;
    double timeArray[timeArrayLen];
    int voltArrayLen = SPIBUFFERSIZE * 1;
    double voltArray[voltArrayLen];
    
    hsize_t dims[1];
    dims[0] = voltArrayLen;
    DataSpace voltSpace(1, dims);
    dims[0] = timeArrayLen;
    DataSpace timeSpace(1, dims);
    FloatType datatype(PredType::NATIVE_DOUBLE);
    datatype.setOrder(H5T_ORDER_LE);
    
    // Init ADC
    adc_config();
    adc_set_chan0();
    adc_set_samplerate(SAMP_RATE_2604);
    
    // Start PRU 1
    prussdrv_exec_program (DRV_PRU, "./text1.bin");
    
    // Time vars
    double startTime, curr_time, lastWrite;
    startTime = curr_time = lastWrite = epoch_double(ptv);
    
    // CLK_GPIO
    int curr_clk, prev_clk;
    curr_clk = prev_clk = 0;
    
    int i;
    while(1){
        curr_clk = readGPIO(CLK_GPIO);
        
        if(curr_clk != prev_clk){
            
            if(readGPIO(STATUS_LED) == 1) {writeGPIO(STATUS_LED, 0);}
            else                          {writeGPIO(STATUS_LED, 1);}
            
            // timestamp
            curr_time = epoch_double(ptv);
            timeArray[0] = curr_time;
            
            // Read available data
            spi_writeread_continuous_transfer(6 + (curr_clk == 0 ? 0 : SPIBUFFERSIZE), SPIBUFFERSIZE, &writeBuffer[0]);
            
            for(i=0;i<SPIBUFFERSIZE;i++) {
                voltArray[i] = adc_GetVoltage(writeBuffer[i]);
            }
            
            // new file and dataset
            filename = FILE_PATH + FILE_PREFIX + to_string(timeArray[0]) + ".h5";
            H5File *file = new H5File(filename, H5F_ACC_TRUNC);
            DataSet *timeSet = new DataSet(file->createDataSet(TIMESET_NAME, datatype, timeSpace));
            DataSet *voltSet = new DataSet(file->createDataSet(VOLTSET_NAME, datatype, voltSpace));
            
            // write to file
            timeSet->write(timeArray, PredType::NATIVE_DOUBLE);
            voltSet->write(voltArray, PredType::NATIVE_DOUBLE);
            
            // close datasets and file
            delete timeSet;
            delete voltSet;
            delete file;
            
            lastWrite = curr_time;
            
        } else {
            curr_time = epoch_double(ptv);
            if(curr_time - lastWrite > timeout) {
                if(system("umount /home/debian/sdcard/") == -1){
                    exit(-1);
                }
                exit(0);
            }
        }
        
        prev_clk = curr_clk;
        
    }
    
    
    return 0;
    
}