/********************************************************
 *     D. Svirida & I. Alekseev, ITEP, Moscow           *
 *------------------------------------------------------*
 *      PP2PP slow control server.                      *
 *      Should be run in the background.                *
 ********************************************************
 * Support fixed set of apparatus.                      *
 * Devices (EAST and WEST):                             *
 * 1) ADAM 5000E with 5017 ADC modules, linked via ADAM *
 * 4570 ETH<->serial - bias and temperature measurement.*
 * 2) SRS 325, liked via INET/GPIB-100 - bias supply    *
 * 3) Silicon power boards, linked via ADAM 4570        *
 * ETH<->serial                                         *
 ********************************************************
 * ADAM 4570 is operated/configured with Advantech      *
 * virtual tty driver. GPIB is operated/configured with *
 * NatInst NI4882 library                               *
 ********************************************************/

#include <fcntl.h>
#include <libconfig.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include "ni4882.h"
#include "pp2pp-slow.h"

#define COM_RETRIES	5
#define COM_TMOUT	100	// ms
#define ONE_WAIT_ON	120  // s
#define TEMP_PANIC  131 // F = 55C

int OneSwitch(int);
// -----   Global variables and types   ----- //

//	SRS devices GPIB conf
typedef struct {
    int dev;	// GPIB branch
    int addr;	// GPIB address
    double vset;	// V, target voltage
    double vrate;	// V/s, ramp speed per adjustment cycle
    int swbits;
} SRSConf;

//	Minimum and maximum values for alarms
typedef struct {
    double min;
    double max;
} MINMAX;

//	Plane components addressing
typedef struct {
    int board;		// address of power board
    int temp;		// address of temperature measurement channel (ADAM)
    int bias;		// address of bias current measurement channel (ADAM)
    int swbits;
} PlaneAddress;

//	Data we got from the configuration
struct {
    char LogFile[1024];	// Path to log file
    int Debug;
    SRSConf SRS[4];	// Bias power modules
    struct {
	    MINMAX AVDD1;   // AVDD1 on silicon power boards
	    MINMAX AVDD2;   // AVDD2 on silicon power boards
	    MINMAX DVDD;    // DVDD on silicon power boards
	    MINMAX DPECL;   // DPECC on silicon power boards
	    MINMAX BIAS;    // each silicon bias measured by ADAM
	    MINMAX TEMP;    // each silicon temperature measured by ADAM    
	    MINMAX VBIAS;   // voltage setting to SRS
	    MINMAX IBIAS;   // current setting to SRS
    } Limits;		// We have common alarm/control limits for all planes
    PlaneAddress Plane[32];
    char SerialFormat[1024];	// format string to make serial devices
    int RS485Port[2];		// RS485 serial port numbers (east/west)
    int AdamPort[2];		// ADAM serial port numbers (east/west)
    int AdamAddr[2];            // ADAM hardware address (east/west)
    int AdamSwbits[2];		// ADAM on/off bits in APC
    char APCaddr[2][20];	// APc ethernet addr (east/west)
    char APCsnmp[2][256];	// format string to form snmp requests for APC (east/west)
    int APCdelay[2][8];		// channel delays in cycles to concider them as ON
    struct {
	int APCrequired[2];	// prerequired APC on
	int APCswitch[2];	// APC to switch on/off
	int SRSswitch;		// SRS mask
	int SiIgnore;		// Silicon ignore mask
    } One;
} Config;

//  Stepping routines
void SRSRead(int);
void AdamRead(int);
void BrdRead(int);
void APCRead(int);

//  Stepping control entry
typedef struct {
    int startcnt;
    int maxcnt;
    void (*call)(int);
} STEntry;

//  Program for the step machine
const STEntry STControl[] = {
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {0, 16, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {16, 32, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {32, 48, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {48, 64, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {64, 80, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {80, 96, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {96, 112, BrdRead},
    {0, 2, APCRead}, {0, 4, SRSRead}, {0, 8, AdamRead}, {112, 128, BrdRead}
};

//	Our running devices, parameters, variables
struct {
    int udp;		// udp file descriptor
    FILE *log;		// log file
    int adam[2];	// ADAM serial ports file descriptors
    int rs485[2];	// RS485 serial ports file descriptors
    int gpib[4];	// GPIB SRS device descriptors
    int Step;       	// step type number - what we are reading now
    int Num;        	// step device number
    time_t oneTime;	// The time one button operation was started
    OneStatus One;	// status and message
    int SiAlarm[32];	// silicon previous alarm
    int SRSAlarm[4];	// SRS previous alarm
    int TermAlarm;	// Temperature previous alarm 
} Run;

DbStruct Db;

// -----   Low level functions   ----- //

//  Open and configure the virtual port
//  raw data, no echo, 8 bit, no parity etc.
//  Return file descriptor
int OpenSerial(int port)
{
    char str[1024];
    int fd;
    struct termios setting;
    
    sprintf(str, Config.SerialFormat, port);
    fd = open(str, O_RDWR | O_NONBLOCK);    // actually we don't need O_NONBLOCK because of terminal settings
    if (fd < 0) return fd;                  // We didn't succeed - just return
//      Set port parameters to very raw processing
    tcgetattr(fd, &setting);
    cfmakeraw(&setting);
    setting.c_oflag &= ~ONLCR;
    setting.c_cc[VMIN] = 0;
    setting.c_cc[VTIME] = 0; 
    cfsetspeed(&setting, B9600);
    tcsetattr(fd, TCSANOW, &setting);
    return fd;
}

//  Check if we got many errors on RS485 communication and 
//  try to close/open/reconfigure the serial port
void CheckRS485(void)
{
    int errcnt[2];
    int i, n;
    
    memset(errcnt, 0, sizeof(errcnt));
    
    for (i=0; i<32; i++) {
        n = ((Config.Plane[i].board / 100) - 1) & 1;
        if (Db.si[i].status == ST_NOCOM) errcnt[n] += 2;
        if (Db.si[i].status == ST_UNREL) errcnt[n]++;
    }
    
    for (i=0; i<2; i++) if (errcnt[i] > 8) {
        close(Run.rs485[i]);
        Run.rs485[i] = OpenSerial(Config.RS485Port[i]);
    }
}

//  Open and configure single GPIB device on branch dev with GPIB address addr
//  Return device descriptor
int OpenGpib(int dev, int addr)
{
    return ibdev(dev, addr, 0, T100ms, 1, 0);
}

//	Init GPIB (NI4882) communication
void InitGpib(void)
{
    int i;
    for (i=0; i<4; i++) Run.gpib[i] = OpenGpib(Config.SRS[i].dev, Config.SRS[i].addr);
}

//	Bind to UDP listenning port
//	Return 0 on success, negative on error
int InitUdp(void)
{
    struct sockaddr_in name;
    int irc;
    
    Run.udp = socket(PF_INET, SOCK_DGRAM, 0);
    if (Run.udp < 0) {
        printf("FATAL - Can not create socket: %m.\n");
        return -1;
    }
    name.sin_family = AF_INET;
    name.sin_port = htons(PPS_UDP_PORT);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    irc = bind(Run.udp, (struct sockaddr *) &name, sizeof(name));
    if (irc < 0) {
        printf("FATAL - Can not bind to port %d: %m.\n", PPS_UDP_PORT);
        printf("The most likely reason is that the server is already running.\n");
        close(Run.udp);
        return -2;
    }
    
    irc = fcntl(Run.udp, F_GETFL, 0);
    if (irc < 0) {
        printf("FATAL - Can not get udp socket parameters: %m.\n");
        close(Run.udp);
        return -3;
    }
    irc |= O_NONBLOCK;
    irc = fcntl(Run.udp, F_SETFL, irc);
    if (irc < 0) {
        printf("FATAL - Can not set udp socket parameters: %m.\n");
        close(Run.udp);
        return -4;
    }

    return 0;
}

//  Sleep time in seconds using nanosleep
void nsleep(double tim)
{
    struct timespec t;
    t.tv_sec = (int) tim;
    t.tv_nsec = 1000000000 * (tim - (int)tim);
    nanosleep(&t, NULL);
}

//  Status to string
const char *St2String(DevStatus st)
{
    static const char ststring[8][6] = {"NOCOM", "OFF", "TRIP", "ALARM", "ON", "UNREL", "POFF", "RAMP"};
    return ststring[st];
}

// Define to which side (E/W) the serial fd belongs
// return: 0/1 - E/W, -1 if not found
int dev2side(int dev)
{
    if (dev == Run.adam[0] || dev == Run.rs485[0]) return 0;
    if (dev == Run.adam[1] || dev == Run.rs485[1]) return 1;
    return -1;
}

//  Send command via ADAM 4570 virtual serial port and get the result
//  dev - file descriptor
//  cmd - the command string
//  buf[blen] - place for the result. If buf = NULL or blen = 0 - don't ask for response.
//  Return the result length. Negative on error.
int SerialCommand(int dev, char *cmd, char *buf, int blen)
{
    int irc;
    int i;
    int len;
    
    irc = dev2side(dev);
    if (irc < 0) return -1;
    // Check if ADAM (eth->ser) is ON
    if ((Config.AdamSwbits[irc] & Db.apc[irc].swmask) != Config.AdamSwbits[irc]) return -100;
    
    irc = write(dev, cmd, strlen(cmd));
    if (irc < 0) return irc;
    if (buf == NULL || blen == 0) return 0; // We don't need reply
    len  = 0;
    for (i = 0; i < COM_RETRIES; i++) {
        nsleep(0.001 * COM_TMOUT);       // Wait TMOUT ms
        irc = read(dev, &buf[len], blen - len);
        if (irc > 0) {
            len += irc;
            if (len == blen) break;
        } else {
            if (len > 0) break;
        }
    }
    return len;
}

// -----   Adam interface   ----- //

#define ADAMRANGE 9
//  Open and configure ADAM devices
//  num - ADAM east/west
int AdamOpen(int num)
{
    int fd;
    char cmd[256], buf[256];
    int i, ierr;
    
    // Check if ADAM (eth->ser) is ON
    if ((Config.AdamSwbits[num] & Db.apc[num].swmask) != Config.AdamSwbits[num]) return -100;

    fd = OpenSerial(Config.AdamPort[num]);
    if (fd < 0) return fd;
    ierr = 0;
    for (i=0; i<4; i++) {
        sprintf(cmd, "$%2.2dS%1.1dA%2.2d00\r", Config.AdamAddr[num], i, ADAMRANGE);
        if (SerialCommand(fd, cmd, buf, sizeof(buf)) <= 0) {
    	    Db.adam5017[num][i] = ST_UNREL;
    	    ierr++;
    	} else {
    	    Db.adam5017[num][i] = ST_ON;
    	}
    }
    Db.adam5000E[num] = (ierr) ? ST_UNREL : ST_ON;
    if (Config.Debug > 90 && Run.log) {
	fprintf(Run.log, "***** ADAM500E[%d] opened. Status = %s\n", num, St2String(Db.adam5000E[num]));
	fflush(Run.log);
    }
    return fd;
}

//  Get proper ADAM reading from the database
//  num = ABC, where digits A, B and C are: A - 1(east)/2(west), B - 0-3 slot number and C - 0-7 input number
double GetAdamValue(int num)
{
    int a, b, c;
    a = (num/100 - 1) & 1;
    b = (num/10 - (num/100)*10) & 3;
    c = (num % 10) & 7;
    return Db.adam[a][b][c];
}

//  Read single ADAM5017 slot and update the database
//  num - slot number: 0-3 - east, 4-7 - west
void AdamRead(int num)
{
    int irc;
    int i;
    int iUNREL, iNOCOM, iPOFF;
    char cmd[256], buf[256], str[8];

    // Check ADAM5000E at the begin of each round status
    if (!(num & 3)) {
	iUNREL = 0;
	iNOCOM = 0;
	iPOFF = 0;
	for (i=0; i<4; i++) {
	    if (Db.adam5017[num/4][i] == ST_NOCOM) iNOCOM++;
	    if (Db.adam5017[num/4][i] == ST_UNREL) iUNREL++;
	    if (Db.adam5017[num/4][i] == ST_POFF) iPOFF++;
	}
	if (iPOFF) {
	    Db.adam5000E[num/4] = ST_POFF;
	}else if (iNOCOM == 4) {
	    Db.adam5000E[num/4] = ST_NOCOM;
	} else if (iNOCOM || iUNREL) {
	    Db.adam5000E[num/4] = ST_UNREL;
	} else {
	    Db.adam5000E[num/4] = ST_ON;
	}
	if (Config.Debug > 90 && Run.log) {
	    fprintf(Run.log, "***** ADAM500E[%d] read check. Status = %s\n", num/4, St2String(Db.adam5000E[num/4]));
	    fflush(Run.log);
	}
    }
    
    if (Run.adam[num/4] < 0) Run.adam[num/4] = AdamOpen(num/4);
    if (Run.adam[num/4] < 0) {
        if (Run.adam[num/4] == -100)
    	    Db.adam5017[num/4][num & 3] = ST_POFF;
    	else
    	    Db.adam5017[num/4][num & 3] = ST_NOCOM;
        return;        
    }
    sprintf(cmd, "#%2.2dS%1.1d\r", Config.AdamAddr[num/4], num & 3);
    memset(buf, 0, sizeof(buf));
    irc = SerialCommand(Run.adam[num/4], cmd, buf, sizeof(buf));
    if (irc < 0) {
        close(Run.adam[num/4]);
        Run.adam[num/4] = -1;
        if (irc == -100)
    	    Db.adam5017[num/4][num & 3] = ST_POFF;
    	else
    	    Db.adam5017[num/4][num & 3] = ST_NOCOM;
        return;        
    }
/*  The return format MUST be:
>12345671234567123456712345671234567123456712345671234567
'>' - the character indicating OK
1234567 - 7-digits * 8 numbers for channels 7 downto 0
*/
    if (irc != 58 || buf[0] != '>') {
        Db.adam5017[num/4][num & 3] = ST_UNREL;
	if (irc) buf[irc-1] = '\0';
// printf("irc = %d [%s]\n", irc, buf);
        return;
    }
    for(i=0; i<8; i++) {
    	memcpy(str, &buf[7*i+1], 7);
		str[7] = '\0';
		Db.adam[num/4][num & 3][7-i] = strtod(str, NULL);
    }
    Db.adam5017[num/4][num & 3] = ST_ON;

    if (num == 7) for (i=0; i<32; i++) {
        Db.si[i].current = GetAdamValue(Config.Plane[i].bias) * 10; // V -> uA on 100kOhm resistor
        Db.si[i].temp = GetAdamValue(Config.Plane[i].temp) * 100;   // The thermometer gives 10mV per F.
		if (Db.si[i].temp > TEMP_PANIC) {
    		if (Run.TermAlarm) {	// We need PANIC twice !!!
			fprintf(Run.log, "Temperature panic !!!\n");
			OneSwitch(0);
			Run.One.status = ST_ALARM;
			memset(Run.One.msg, 0, sizeof(Run.One.msg));
			sprintf(Run.One.msg, "Temperature panic !!!");
		    }
		    Run.TermAlarm++;
		} else {
		    Run.TermAlarm = 0;
		}
    }
}

// -----   Power boards interface   ----- //
//  Get proper serial device descriptor
int BrdNum2fd(int num)
{
    return Run.rs485[((Config.Plane[num].board / 100) - 1) & 1];
}

//  Read the board and update the database
//  num = 4*<board number> + <output number>
//  When getting data for <output> = 0 we also take
//  to the Database ADAM current and temperature readings
void BrdRead(int num)
{
    int irc;
    char cmd[256], buf[256];
    int n, m;
    char *ptr;
    double val;
    
    n = num >> 2;  // board number
    m = num & 3;
    val = NAN;
    // Check if board is ON
    if ((Config.Plane[n].swbits & Db.apc[((Config.Plane[n].board / 100) - 1) & 1].swmask) == Config.Plane[n].swbits) {
	sprintf(cmd, "$%2.2X%c\r", Config.Plane[n].board % 100, 'A' + m);
	irc = SerialCommand(BrdNum2fd(n), cmd, buf, sizeof(buf) - 1);
	if (irc <= 0) {
    	    Db.si[n].status = ST_NOCOM;
	} else if (irc < 5) {    // we got something too short
    	    Db.si[n].status = ST_UNREL;        
	} else {
    	    buf[irc] = '\0';
    //  We have at least 3 variations of boards with quite different output.
    //  We will look for dot and numerical characters around it.
    //  This would be our float number
    	    ptr = strchr(buf, '.');
    	    if (!ptr) {       // we can not decode this
        	Db.si[n].status = ST_UNREL;        
    	    } else {
        	ptr--;
        	for (; ptr >= buf; ptr--) if (!isdigit(*ptr)) break;
        	ptr++;
        	val = strtod(ptr, NULL);
    	    }
	}
    } else Db.si[n].status = ST_POFF;
    switch(m) {
    case 0:
        Db.si[n].AVDD1 = val;
        break;
    case 1:
        Db.si[n].AVDD2 = val;
        break;
    case 2:
        Db.si[n].DVDD = val;
        break;
    case 3:
        Db.si[n].DPECL = val;
        break;
    }

    if (m == 3) {       // Update status
        if (!isnan(Db.si[n].AVDD1) && !isnan(Db.si[n].AVDD2) && !isnan(Db.si[n].DVDD) && !isnan(Db.si[n].DPECL)) {
            Db.si[n].status = (Db.si[n].AVDD1 < 1.0 && Db.si[n].AVDD2 < 1.0 && Db.si[n].DVDD < 1.0 && Db.si[n].DPECL < 1.0) ? ST_OFF : ST_ON;
            if (Db.si[n].status == ST_ON) {
                if ((Db.si[n].current > Config.Limits.BIAS.max && Db.srs[n/8].status == ST_ON) ||			// check if SRS is already ramped
		    Db.si[n].temp > Config.Limits.TEMP.max ||
                    Db.si[n].AVDD1 > Config.Limits.AVDD1.max  || Db.si[n].AVDD1 < Config.Limits.AVDD1.min || 
                    Db.si[n].AVDD2 > Config.Limits.AVDD2.max  || Db.si[n].AVDD2 < Config.Limits.AVDD2.min || 
                    Db.si[n].DVDD > Config.Limits.DVDD.max    || Db.si[n].DVDD < Config.Limits.DVDD.min || 
                    Db.si[n].DPECL > Config.Limits.DPECL.max  || Db.si[n].DPECL < Config.Limits.DPECL.min) {
                    if (Run.SiAlarm[n]) Db.si[n].status = ST_ALARM;		// Only two alarm insequence produce real alarm
		    Run.SiAlarm[n]++;
		} else {
		    Run.SiAlarm[n] = 0;
		}
            }
        }
    }

//    printf("n=%d m=%d brd=%d cmd=%s\n irc=%d buf=%s\n ptr=%s\n val=%f\n", 
//        n, m, Config.Plane[n].board, cmd, irc, (irc > 5) ? buf : "XX",  (irc > 5) ? ptr : "XX", val);
}

//  Make log entry for all board information, including bias currents and temperature read by ADAM
void Brd2Log(FILE *f)
{
    int i;
    for(i=0; i<32; i++) fprintf(f, "%2d: %5s %4.2fV %4.2fV %4.2fV %4.2fV %3.0fF %4.0fuA%c",
        i, St2String(Db.si[i].status), Db.si[i].AVDD1, Db.si[i].AVDD2, Db.si[i].DVDD, 
        Db.si[i].DPECL, Db.si[i].temp, Db.si[i].current, ((i & 3) == 3) ? '\n' : ' ');

    FILE *fepic = fopen("ToEPICS.txt", "wt") ;
    for(i=0; i<32; i++) fprintf(fepic, "%4.2f %4.2f %4.2f %4.2f %3.0f %4.0f%c",
        Db.si[i].AVDD1, Db.si[i].AVDD2, Db.si[i].DVDD,
        Db.si[i].DPECL, Db.si[i].temp, Db.si[i].current, ((i & 3) == 3) ? '\n' : ' ');
    fclose(fepic) ;

}

//  Switch on outptus at board num
//  Return 0 on success, negative on error.
//  We will not wait for the board answer, as it will switch
//  the powers on according to the required sequence and will
//  send the answer only after - 1-2 s later.
int BrdOn(int num)
{
    int irc;
    char cmd[256];
    
    sprintf(cmd, "$%2.2X1\r", Config.Plane[num].board % 100);
    return SerialCommand(BrdNum2fd(num), cmd, NULL, 0);
}

//  Switch off outptus at board num
//  Return 0 on success, negative on error.
int BrdOff(int num)
{
    int irc;
    char cmd[256], buf[256];
    
    sprintf(cmd, "$%2.2X0\r", Config.Plane[num].board % 100);
    irc = SerialCommand(BrdNum2fd(num), cmd, buf, sizeof(buf));
    if (irc >= 0) return 0;
    return irc;
}

//  Switch On/off board num. what = 0 - off, others - on.
//  Return 0 on success, negative on error.
//  We always send the requested command.
int BrdSwitch(int num, int what)
{
    if (num < 0 || num >= 32) return -1;    // wrong board number
    return (what) ? BrdOn(num) : BrdOff(num);
}

//  Adjust board voltages. 
//  num - board number
//  what 0-3 output to adjust (AVDD1, AVDD2, DVDD, DPECL)
//  inc - >0 - increment, <0 - decrement, =0 - do nothing
//  Return 0 on success, negative on error
int BrdAdjust(int num, int what, int inc)
{
    char Ch;
    int irc;
    char cmd[256], buf[256];

    if (num < 0 || num >= 32) return -1;    // wrong board number
    if (what <0 || what >= 4) return -2;    // wrong output number
    if (inc == 0) return 0;                 // nothing to do, but this is OK.
    Ch = 'E' + 2*what;
    if (inc < 0) Ch++;
    sprintf(cmd, "$%2.2X%c\r", Config.Plane[num].board % 100, Ch);
    irc = SerialCommand(BrdNum2fd(num), cmd, buf, sizeof(buf));
    if (irc >= 0) return 0;
    return irc;
}

// -----   SRS GPIB interface   ----- //

//  Make a log record for SRS devices to stream f
void SRS2Log(FILE *f)
{
    int i;
    fprintf(f, "SRS: ");
    for(i=0; i<4; i++) fprintf(f, "%d: %5s SET=%4.0fV V=%4.0fV I=%5.0fuA ", 
        i, St2String(Db.srs[i].status), Db.srs[i].vset, Db.srs[i].vout, Db.srs[i].iout);
    fprintf(f, "\n");
}

//  Send command via GPIB and get the result
//  dev - SRS unit - index in Config.SRS[] array
//  cmd - the command string
//  buf[blen] - place for the result. If buf = NULL or blen = 0 - don't ask for response.
//  Return the result length. Negative on error.
int SRSCommand(int dev, char *cmd, char *buf, int blen)
{
    int i, irc;
    int ibcnt_s;
    
    // Check if SRS is ON and we can connect, i.e. GPIB connection is ON
    if ((Config.SRS[dev].swbits & Db.apc[Config.SRS[dev].dev].swmask) != Config.SRS[dev].swbits) return -101;

    if (Run.gpib[dev] < 0) Run.gpib[dev] = OpenGpib(Config.SRS[dev].dev, Config.SRS[dev].addr);
    if (Run.gpib[dev] < 0) return -10;   // Can not open the device
    irc = ibwrt(Run.gpib[dev], cmd, strlen(cmd));
    if (irc & ERR) {
        Run.gpib[dev] = -1;   // Invalidate handle on error - we will reopen it next time
        return -11;
    }
    if (Config.Debug > 80 && Run.log) {
	fprintf(Run.log, "***** SRS command: %s", cmd);
	fflush(Run.log);
    }
    if (buf == NULL || blen == 0) return 0; // We don't need reply
    for(;;) {
	irc = ibrd(Run.gpib[dev], buf, blen);
	if (irc & (ERR | TIMO)) break;		// try to read until timeout 
	ibcnt_s = ibcnt;
    }
    if ((irc & ERR) && !(irc & TIMO)) {
        Run.gpib[dev] = -1;   // Invalidate handle on error - we will reopen it next time
        return -12;
    }
    if (Config.Debug > 80 && Run.log) {
	fprintf(Run.log, "***** SRS reply: ");
	for (i=0; i<ibcnt_s; i++) fprintf(Run.log, "%c", buf[i]);
	fprintf(Run.log, "\n");
	fflush(Run.log);
    }
    return ibcnt_s;
}

//  Switch OFF SRS device dev and sets 0 as VSET.
//  Return 0 on OK, negative on communication error.
int SRSOff(int dev)
{
    return SRSCommand(dev, "VSET0;HVOF\n", NULL, 0);
}

//  Switch ON SRS device dev.
//  val - initial voltage
//  We also program here all limits etc.
//  Trip current is set as limit + 30%
int SRSOn(int dev, double val)
{
    char cmd[256];
    sprintf(cmd, "TMOD0;ILIM%6.1fE-6;ITRP%6.1fE-6;VLIM%5.1f;VSET%5.1f;TCLR;HVON\n", 
        Config.Limits.IBIAS.max, 1.3*Config.Limits.IBIAS.max, Config.Limits.VBIAS.max, val);
    return SRSCommand(dev, cmd, NULL, 0);
}

//  Set new voltage for SRS device dev.
//  val - new voltage in Volts.
//  Return 0 on OK, negative on communication error.
int SRSSet(int dev, double val)
{
    char cmd[256];
    sprintf(cmd, "VSET%5.1f\n", val);
    return SRSCommand(dev, cmd, NULL, 0);    
}

#define SIGN(A) (((A) > 0) ? 1 : (((A) < 0) ? -1 : 0))

//  Get SRS dev data and update the database
//  We also do ramping here!
void SRSRead(int dev)
{
    int irc;
    char buf[256];
    char *tok;
    
    if (dev < 0 || dev >= 4) return;	// invalid number
    Db.srs[dev].vset = NAN;
    Db.srs[dev].vout = NAN;
    Db.srs[dev].iout = NAN;
    // Check if SRS is ON and we can connect, i.e. GPIB connection is ON
    if ((Config.SRS[dev].swbits & Db.apc[Config.SRS[dev].dev].swmask) == Config.SRS[dev].swbits) {
	irc = SRSCommand(dev, "VSET?;VOUT?;IOUT?;*STB?\n", buf, sizeof(buf) - 1);
	if (irc <= 0) {
    	    Db.srs[dev].status = ST_NOCOM;
    	    return;
	}
	buf[irc] = '\0';
	tok = strtok(buf, ";");
	if (tok) Db.srs[dev].vset = strtod(tok, NULL);
	tok = strtok(NULL, ";");
	if (tok) Db.srs[dev].vout = strtod(tok, NULL);
	tok = strtok(NULL, ";");
	if (tok) Db.srs[dev].iout = strtod(tok, NULL) * 1.0E6;  // we got in Amps.
	tok = strtok(NULL, ";");
	irc = (tok) ? strtol(tok, NULL, 10) : 0;
	if (irc & 6) {      // Itrip or Vtrip
    	    Db.srs[dev].status = ST_TRIP;
	} else if (irc & 0x80) {
    	    Db.srs[dev].status = ST_ON;
	} else {
    	    Db.srs[dev].status = ST_OFF;
	}
    } else Db.srs[dev].status = ST_POFF;
    
    if ((Db.srs[dev].status == ST_ON) && (Db.srs[dev].iout > Config.Limits.IBIAS.max || 
        Db.srs[dev].vout > Config.Limits.VBIAS.max)) {
	if (Run.SRSAlarm[dev]) Db.srs[dev].status = ST_ALARM;
	Run.SRSAlarm[dev]++;
    }
    if (Db.srs[dev].status == ST_ON) Run.SRSAlarm[dev] = 0;
    if ((Db.srs[dev].status == ST_ON) && (Db.srs[dev].vset != Db.srs[dev].vtarg)) {
	if (fabs(Db.srs[dev].vset - Db.srs[dev].vtarg) < Config.SRS[dev].vrate) {
	    SRSSet(dev, Db.srs[dev].vtarg);
	} else {
	    SRSSet(dev, Db.srs[dev].vset + SIGN(Db.srs[dev].vtarg - Db.srs[dev].vset) * Config.SRS[dev].vrate);
	}
	Db.srs[dev].status = ST_RAMP;
    }
}

//  Get voltage setting for the SRS dev
//  Return posiitve value for real voltage, zero for the 
//  switched off device and negative on communication error.
double SRSGetSet(int dev)
{
    int irc;
    char buf[256];
    char *ptr;
    double val;
    
    irc = SRSCommand(dev, "VSET?;*STB?\n", buf, sizeof(buf) - 1);
    if (irc < 0) return irc;
    if (irc == 0) return -10.0;
    buf[irc] = '\0';
    printf("SRS: %s\n");
    val = strtod(buf, NULL);
    ptr = strchr(buf, ';');
    if (!ptr) return -20.0;
    ptr++;
    irc = strtol(ptr, NULL, 10);
    return (irc & 0x80) ? val : 0.0;
}

//  Initial check if SRS is already ON
//  Set vtarg either from SRS (if on) or from config (if it was off)
//  We do retires here as this is important to reach the device if ever possible
void SRSCheck(void)
{
    int i, j;
    for (i=0; i<COM_RETRIES; i++) {	// Retries
	for (j=0; j<4; j++) {	// Cycle over SRS
	    if (Db.srs[j].vtarg > 1) continue;	// This channel we already got. Bias below 1 V is not considered
	    Db.srs[j].vset = SRSGetSet(j);
	    Db.srs[j].vtarg = Db.srs[j].vset;
	}
    }
    if (Run.log) {
	fprintf(Run.log, "SRS check: VON = %6.1f %6.1f %6.1f %6.1f\n", Db.srs[0].vset, Db.srs[1].vset, Db.srs[2].vset, Db.srs[3].vset);
	fflush(Run.log);
    }
    for (j=0; j<4; j++) if (Db.srs[j].vtarg < 1) Db.srs[j].vtarg = Config.SRS[j].vset;
    if (Run.log) {
	fprintf(Run.log, "SRS check: VTARG = %6.1f %6.1f %6.1f %6.1f\n", Db.srs[0].vtarg, Db.srs[1].vtarg, Db.srs[2].vtarg, Db.srs[3].vtarg);
	fflush(Run.log);
    }
}

//  Switch On/off SRS num. what = 0 - off, others - on.
//  Return 0 on success, negative on error, positive on warning
//  We switch off immediately and always send the command
//  We ignore switch on if the device is alreayd on
int SRSSwitch(int num, int what)
{
    double v;
    int i;
    if (num < 0 || num >= 4) return -1;     // wrong SRS number
    if (!what) return SRSOff(num);
    for (i=0; i < COM_RETRIES; i++) {
	v = SRSGetSet(num);
	if (v >= 0) break;
    }
    if (v < 0) return -2;		    // We can't reach the device 
    if (v > 1) return 0;		    // nothing to do
    return SRSOn(num, Config.SRS[num].vrate);		// start ramping up - the first step
}

// ----- APC outlet switch ----- //

// Reads switched mask from APC on one side
void APCRead(int num) {
    FILE * fcmd;
    char str[256], res[1024];
    int i, rc, swmask;
    char *ptr;

    // If something is wrong, assume ON
    if (!strlen(Config.APCsnmp[num]) || !strlen(Config.APCaddr[num])) {
	Db.apc[num].swmask = 0xFF;
	return;
    }
    swmask = 0;
    sprintf(str,"snmpwalk -v1 -c public %s %s 2>&1", Config.APCaddr[num], Config.APCsnmp[num]);
    fcmd = popen(str, "r");
    if (fcmd) { 
	rc = fread(res, 1, sizeof(res)-1, fcmd);
	res[rc]='\0';
	fclose(fcmd);
	ptr = res;
	for (i=0; i<8; i++) {
	    // The obtained strings should end with "INTEGER: n", n=1 for ON, n=2 for OFF
	    ptr = (char *)strcasestr(ptr, "INTEGER:");
	    if (!ptr) { swmask |= (1 << i); continue;}
	    ptr = strchr(ptr,':');
	    ptr++;
	    rc = 2 - strtoul(ptr, NULL, 0); // Only OFF will give 0
	    if (rc) swmask |= (1 << i); 
	}
    } else swmask = 0xFF;    
    for (i=0; i<8; i++) {
	if (swmask & (1 << i)) {
	    if (Db.apc[num].delay[i] == 0) {
		Db.apc[num].delay[i] = time(NULL);
	    } else if (time(NULL) - Db.apc[num].delay[i] > Config.APCdelay[num][i]) {
		Db.apc[num].swmask |= (1 << i);
	    } 
	} else {
	    // show OFF immediately
	    Db.apc[num].swmask &= ~(1 << i);
	    Db.apc[num].delay[i] = 0;
	}
    }
//printf("Read mask %X, seen mask %X\n", swmask, Db.apc[num].swmask);
}

int APCSwitch(int num, int chan, int what) {
    FILE * fcmd;
    char str[256], res[1024];
    int i, rc;
    char *ptr;

    // If something is wrong, assume ON
    if (!strlen(Config.APCsnmp[num]) || !strlen(Config.APCaddr[num])) return 0;

    sprintf(str,"snmpset -v1 -c private %s %s.%1.1d int %d 2>&1", Config.APCaddr[num], Config.APCsnmp[num], chan, (what) ? 1 : 2);
    fcmd = popen(str, "r");
    if (!fcmd) return -1;
    rc = fread(res, 1, sizeof(res)-1, fcmd);
    res[rc]='\0';
    fclose(fcmd);
    // The obtained string should end with "INTEGER: n", n=1 for ON, n=2 for OFF
    ptr = (char *)strcasestr(res, "INTEGER:");
    if (!ptr) return -1;
    ptr = strchr(ptr,':');
    ptr++;
    rc = 2 - strtoul(ptr, NULL, 0); // Only OFF will give 0
    if (rc == what) return 1; 
    return 0;
}

//	initial check of one button status
//	Check all required outlets and bias
void OneInit(void)
{
    int mask, i;
    Run.One.status = ST_OFF;
    memset(Run.One.msg, 0, sizeof(Run.One.msg));
    for (i=0; i<2; i++) {
    	    mask = Config.One.APCrequired[i] | Config.One.APCswitch[i];
    	    if ((Db.apc[i].swmask & mask) != Config.One.APCrequired[i]) return;
    }
    for (i=0; i<4; i++) if ((Config.One.SRSswitch & (1 << i)) && (Db.srs[i].vset < Config.SRS[i].vrate)) return;
    Run.One.status = ST_ON;
}

//	One button on/off
int OneSwitch(int what)
{
    int i, j;
	memset(Run.One.msg, 0, sizeof(Run.One.msg));
    if (what && Run.One.status == ST_OFF) {
//	Check prerequired
    	for (i=0; i<2; i++) {
    	    APCRead(i);
    	    if ((Db.apc[i].swmask & Config.One.APCrequired[i]) != Config.One.APCrequired[i]) {
            	fprintf(Run.log, "One button: switch on requested with some required APC %s channels off\n", (i) ? "West" : "East");
				fprintf(Run.log, "One button: APC mask = %2.2X - prerequested mask = %2.2X\n", Db.apc[i].swmask, Config.One.APCrequired[i]);
				fflush(Run.log);
				sprintf(&Run.One.msg[strlen(Run.One.msg)], 
					"Some of the required APC %s channels are off. Go to APC web-page at %s or call expert. ",
					(i) ? "West" : "East", Config.APCaddr[i]);
				Run.One.status = ST_ALARM;
			}
        }
		if (Run.One.status == ST_ALARM) return 1;
//	Switch everything on
		for (i=0; i<2; i++) for (j=0; j<8; j++) if ((Config.One.APCswitch[i] & (1 << j)) && (APCSwitch(i, j+1, 1) != 1)) {
			fprintf(Run.log, "One button: switch of APC %s outlet %d failed.\n", (i) ? "West" : "East", j+1);
			fflush(Run.log);
			sprintf(&Run.One.msg[strlen(Run.One.msg)], 
				"Can not turn on APC %s outlet %d. Close the APC web page, please, or repeat the attempt in 3 minutes. ",
				(i) ? "West" : "East", j+1);
			Run.One.status = ST_ALARM;
		}
		if (Run.One.status == ST_ALARM) return 2;
		for (i=0; i<4; i++) if (Config.One.SRSswitch & (1 << i)) for (j=0; j<COM_RETRIES; j++) if (!SRSSwitch(i, 1)) break;
		Run.One.status = ST_RAMP;
		Run.oneTime = time(NULL);
		fprintf(Run.log, "One button: switching on ...\n");
		fflush(Run.log);
    } else if (what && Run.One.status == ST_ALARM) {
		sprintf(Run.One.msg, "Press OFF before the next attepmt.");
		return 3;
	} else if (!what) {
		for (i=0; i<2; i++) for (j=0; j<8; j++) if (Config.One.APCswitch[i] & (1 << j)) APCSwitch(i, j+1, 0);
		for (i=0; i<4; i++) if (Config.One.SRSswitch & (1 << i)) for (j=0; j<COM_RETRIES; j++) if (!SRSSwitch(i, 0)) break;
		fprintf(Run.log, "One button: switching off ...\n");
		fflush(Run.log);
		Run.One.status = ST_OFF;
	}
    return 0;
}

//	One button status check
void OneCheck(void)
{
	int i;
	if (Run.One.status == ST_RAMP && time(NULL) - Run.oneTime > ONE_WAIT_ON) Run.One.status = ST_ON;
	if (Run.One.status == ST_ALARM) Run.One.status = ST_ON;	// try ON - may be alarm is gone.
	if (Run.One.status == ST_ON) {
		memset(Run.One.msg, 0, sizeof(Run.One.msg));
		for (i=0; i<4; i++) if ((Config.One.SRSswitch & (1 << i)) && (Db.srs[i].status == ST_ALARM || Db.srs[i].status == ST_TRIP)) {
			sprintf(&Run.One.msg[strlen(Run.One.msg)], "SRS %d. ", i);
			Run.One.status = ST_ALARM;
		}
		for (i=0; i<32; i++) if (!(Config.One.SiIgnore & (1 << i)) && Db.si[i].status == ST_ALARM) {
			sprintf(&Run.One.msg[strlen(Run.One.msg)], "Brd %d. ", i);
			Run.One.status = ST_ALARM;
		}
		if (Run.One.status == ST_ALARM) {
			sprintf(&Run.One.msg[strlen(Run.One.msg)], "- Listed device(s) are in bad status. Call expert or try OFF/ON no more than once.");
		} else {
			memset(Run.One.msg, 0, sizeof(Run.One.msg));	// clear message for good status
		}
	}
}

// -----   General top level functions   ----- //

//	Read configuration from the file confname
//	Return 0 on success, negative on error
int ReadConfig(char *confname)
{
    config_t cnf;
    long tmp;
    double dtmp;
    char *stmp;
    char str[1024];
    int i, j;
//		Read configuration file
    config_init(&cnf);
    if (config_read_file(&cnf, confname) != CONFIG_TRUE) {
	    printf("Configuration error in file %s at line %d: %s\n", 
	        confname, config_error_line(&cnf), config_error_text(&cnf));
	    return -1;
    }
//	Log file
    strncpy(Config.LogFile, config_lookup_string(&cnf, "Log", (const char **)&stmp) ? stmp : "pp2pp-slow.log", sizeof(Config.LogFile));
    if (strcasecmp(Config.LogFile, "NONE")) {
        Run.log = fopen(Config.LogFile, "at");
        if (!Run.log) {
            printf("Can not open log-file %s. Logging to stdout.\n", Config.LogFile);
            Run.log = stdout;
        }
    }
    Config.Debug = config_lookup_int(&cnf, "Debug", &tmp) ? tmp : 0;
//	SRS gpib
    for (i=0; i<4; i++) {
        sprintf(str, "Bias.SRS%1.1d.dev", i);
        Config.SRS[i].dev = config_lookup_int(&cnf, str, &tmp) ? tmp : i >> 1;
        sprintf(str, "Bias.SRS%1.1d.addr", i);
        Config.SRS[i].addr = config_lookup_int(&cnf, str, &tmp) ? tmp : 5 + (i & 1);
        sprintf(str, "Bias.SRS%1.1d.vset", i);
        Config.SRS[i].vset = config_lookup_float(&cnf, str, &dtmp) ? dtmp : 50.;
        sprintf(str, "Bias.SRS%1.1d.vrate", i);
        Config.SRS[i].vrate = config_lookup_float(&cnf, str, &dtmp) ? dtmp : 5.;
        sprintf(str, "Bias.SRS%1.1d.swbits", i);
        Config.SRS[i].swbits = config_lookup_int(&cnf, str, &tmp) ? tmp : 0;
    }
//  Limits
    Config.Limits.AVDD1.min = config_lookup_float(&cnf, "Limits.AVDD1.min", &dtmp) ? dtmp : 5.3;
    Config.Limits.AVDD1.max = config_lookup_float(&cnf, "Limits.AVDD1.max", &dtmp) ? dtmp : 5.7;
    Config.Limits.AVDD2.min = config_lookup_float(&cnf, "Limits.AVDD2.min", &dtmp) ? dtmp : 3.5;
    Config.Limits.AVDD2.max = config_lookup_float(&cnf, "Limits.AVDD2.max", &dtmp) ? dtmp : 3.7;
    Config.Limits.DVDD.min = config_lookup_float(&cnf, "Limits.DVDD.min", &dtmp) ? dtmp : 5.0;
    Config.Limits.DVDD.max = config_lookup_float(&cnf, "Limits.DVDD.max", &dtmp) ? dtmp : 5.3;
    Config.Limits.DPECL.min = config_lookup_float(&cnf, "Limits.DPECL.min", &dtmp) ? dtmp : 5.1;
    Config.Limits.DPECL.max = config_lookup_float(&cnf, "Limits.DPECL.max", &dtmp) ? dtmp : 5.3;
    Config.Limits.BIAS.min = config_lookup_float(&cnf, "Limits.BIAS.min", &dtmp) ? dtmp : -0.1;
    Config.Limits.BIAS.max = config_lookup_float(&cnf, "Limits.BIAS.max", &dtmp) ? dtmp : 20.0;
    Config.Limits.TEMP.min = config_lookup_float(&cnf, "Limits.TEMP.min", &dtmp) ? dtmp : -0.1;
    Config.Limits.TEMP.max = config_lookup_float(&cnf, "Limits.TEMP.max", &dtmp) ? dtmp : 100.0;
	if (Config.Limits.TEMP.max > TEMP_PANIC - 5) Config.Limits.TEMP.max = TEMP_PANIC - 5;		// ensure the value is reasonable
    Config.Limits.VBIAS.min = config_lookup_float(&cnf, "Limits.VBIAS.min", &dtmp) ? dtmp : 5.0;
    Config.Limits.VBIAS.max = config_lookup_float(&cnf, "Limits.VBIAS.max", &dtmp) ? dtmp : 150.0;
    Config.Limits.IBIAS.min = config_lookup_float(&cnf, "Limits.IBIAS.min", &dtmp) ? dtmp : 0.0;
    Config.Limits.IBIAS.max = config_lookup_float(&cnf, "Limits.IBIAS.max", &dtmp) ? dtmp : 200.0;
//  Planes
    for (i=0; i<32; i++) {
        sprintf(str, "Silicon.Plane%2.2d.board", i);
        Config.Plane[i].board = config_lookup_int(&cnf, str, &tmp) ? tmp : ((i>>4) & 1) * 100 + (i & 0xF);
        sprintf(str, "Silicon.Plane%2.2d.temp", i);
        Config.Plane[i].temp = config_lookup_int(&cnf, str, &tmp) ? tmp : ((i>>4) & 1) * 100 + ((i>>3) & 1) * 20 + (i & 7);
        sprintf(str, "Silicon.Plane%2.2d.bias", i);
        Config.Plane[i].bias = config_lookup_int(&cnf, str, &tmp) ? tmp : ((i>>4) & 1) * 100 + ((i>>3) & 1) * 20 + (i & 7) + 10;
        sprintf(str, "Silicon.Plane%2.2d.swbits", i);
        Config.Plane[i].swbits = config_lookup_int(&cnf, str, &tmp) ? tmp : 0;
    }
//  Serial & ADAM
    strncpy(Config.SerialFormat, config_lookup_string(&cnf, "Serial.name", (const char **)&stmp) ? stmp : "/dev/vttyAP%d", sizeof(Config.SerialFormat));
    for (i=0; i<2; i++) {
        sprintf(str, "Serial.board%d", i);
        Config.RS485Port[i] = config_lookup_int(&cnf, str, &tmp) ? tmp : 2*i + 1;
        sprintf(str, "Serial.adam%d", i);
        Config.AdamPort[i] = config_lookup_int(&cnf, str, &tmp) ? tmp : 2*(i + 1);
        sprintf(str, "ADAM.ADAM%d.addr", i);
        Config.AdamAddr[i] = config_lookup_int(&cnf, str, &tmp) ? tmp : 1;
        sprintf(str, "ADAM.ADAM%d.swbits", i);
        Config.AdamSwbits[i] = config_lookup_int(&cnf, str, &tmp) ? tmp : 0;
    }
// APC
    for (i=0; i<2; i++) {
        sprintf(str, "Outlets.APC%1.1d.addr", i);
	strncpy(Config.APCaddr[i], config_lookup_string(&cnf, str, (const char **)&stmp) ? stmp : "", sizeof(Config.APCaddr[0]));
        sprintf(str, "Outlets.APC%1.1d.snmp", i);
	strncpy(Config.APCsnmp[i], config_lookup_string(&cnf, str, (const char **)&stmp) ? stmp : "", sizeof(Config.APCsnmp[0]));
	for (j=0; j<8; j++) {
    	    sprintf(str, "Outlets.APC%1.1d.delay%1.1d", i, j+1);
    	    Config.APCdelay[i][j] = config_lookup_int(&cnf, str, &tmp) ? tmp : 0;
	    Db.apc[i].delay[j] = Config.APCdelay[i][j];
	}
    }
//  One
    Config.One.APCrequired[0] = config_lookup_int(&cnf, "One.APC0.required", &tmp) ? tmp : 0x7C;
    Config.One.APCrequired[1] = config_lookup_int(&cnf, "One.APC1.required", &tmp) ? tmp : 0x7C;
    Config.One.APCswitch[0] = config_lookup_int(&cnf, "One.APC0.switch", &tmp) ? tmp : 3;
    Config.One.APCswitch[1] = config_lookup_int(&cnf, "One.APC1.switch", &tmp) ? tmp : 3;
    Config.One.SRSswitch = config_lookup_int(&cnf, "One.SRS", &tmp) ? tmp : 0xF;
    Config.One.SiIgnore = config_lookup_int(&cnf, "One.SiIgnore", &tmp) ? tmp : 0;

//  Destroy configuration structure 
    config_destroy(&cnf);
    return 0;
}

//	Initialize our database and variables
void InitDb(void)
{
    int i;
//      Run
    memset(&Run, 0, sizeof(Run));
//  Make illegal file descriptors
    Run.udp = -1;
    for (i = 0; i < 2; i++) {
        Run.adam[i] = -1;
        Run.rs485[i] = -1;
    }
    for (i = 0; i < 4; i++) Run.gpib[i] = -1;
//      Db
    memset(&Db, 0, sizeof(Db));
    Db.len = sizeof(Db);
}

//	Serve UDP - get and process requests
//	ServeUdp reads udp port until empty return.
//	Return 1 if restart was signalled, 0 - otherwise
int ServUdp(void)
{
    UdpIn cmd;
    struct sockaddr_in addr;
    int irc, len;
    for(;;) {
        len = sizeof(addr);
        irc = recvfrom(Run.udp, &cmd, sizeof(cmd), 0, (struct sockaddr *)&addr, &len);
        if (irc <= 0) break;
        if (irc != sizeof(cmd)) {
            printf("Wrong udp command received. Ignoring.\n");
            break;
        }
        if (len != sizeof(addr) || addr.sin_family != AF_INET) {
            printf("Strange udp source address. Ignoring.\n");
            break;
        }
        //  Debug print 
        if (Config.Debug > 10 && Run.log) {
    	    fprintf(Run.log, "***** Command %d (%d %d %d %d %d) from %s:%d received.\n", 
        	cmd.cmd, cmd.par[0], cmd.par[1], cmd.par[2], cmd.par[3],
    		cmd.par[4], inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    	    fflush(Run.log);
    	}
        switch(cmd.cmd) {
        case cmd_echo:      // just return the block
            sendto(Run.udp, &cmd, sizeof(cmd), 0, (struct sockaddr *)&addr, len);
            break;
        case cmd_getdb:     // send the whole database
            sendto(Run.udp, &Db, sizeof(Db), 0, (struct sockaddr *)&addr, len);
            break;
        case brd_switch:    // translate this command to the board and return integer status 
            irc = BrdSwitch(cmd.par[0], cmd.par[1]);
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            break;    
        case brd_adjust:    // translate this command to the board and return integer status 
            irc = BrdAdjust(cmd.par[0], cmd.par[1], cmd.par[2]);
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            break;
        case srs_switch:    // translate this command to the SRS or start ramping and return integer status 
            irc = SRSSwitch(cmd.par[0], cmd.par[1]);
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            break;        
        case srs_set:      // change current SRS setting by either ramping if ON, or just internally 
	    Db.srs[cmd.par[0]].vtarg = cmd.par[1];		// we memorize the setting and deal with it during regular cycle
    	    irc = 0;
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            break;        
        case apc_switch:    // immediately send snmp request for apc chans, p0 - e/w, p1 - chan #, p2 - off/on 
            irc = APCSwitch(cmd.par[0], cmd.par[1], cmd.par[2]);
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            break;        
        case cmd_restart:    // restart the server
            irc = 0;
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            return 1;
	case one_switch:
            irc = OneSwitch(cmd.par[0]);
            sendto(Run.udp, &irc, sizeof(irc), 0, (struct sockaddr *)&addr, len);
            break;
	case one_status:
	    OneCheck();
            sendto(Run.udp, &Run.One, sizeof(OneStatus), 0, (struct sockaddr *)&addr, len);
            break;
        default:            // do nothing - ignore strange commands
            break;
        }
    }
    return 0;
}

//	Do a step in servicing device read/write
//	Return 1 on the last step, zero - otherwise
int OneStep(void)
{
    int irc;
    
    irc = 0;
    Db.step = Run.Step;
    Db.maxstep = sizeof(STControl) / sizeof(STControl[0]);
    STControl[Run.Step].call(Run.Num);
    Run.Num++;
//    printf("."); fflush(stdout);
    if (Run.Num >= STControl[Run.Step].maxcnt) {
        Run.Step++;
        if (Run.Step >= sizeof(STControl) / sizeof(STControl[0])) {
            Run.Step = 0;
//	    printf("\n"); fflush(stdout);
            irc = 1;
        }
        Run.Num = STControl[Run.Step].startcnt;
    }
    return irc;
}

//  Write measurment to log-file stream f
void DoLog(FILE *f)
{
    time_t tm;
    
    tm = time(NULL);   
    fprintf(f, "===== %s", ctime(&tm));
    SRS2Log(f);
    Brd2Log(f);
    fflush(f);
}

//	Initialize serial (ADAM 4570) communication
void InitSerial(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        Run.adam[i] = AdamOpen(i);
        Run.rs485[i] = OpenSerial(Config.RS485Port[i]);
    }
}

//	Close all files...
void CloseAll(void)
{
    int i;
    if (Run.udp >= 0) close(Run.udp);
    if (Run.log) fclose(Run.log);
    for (i=0; i<2; i++) {
	if (Run.adam[i] >= 0) close(Run.adam[i]);
	if (Run.rs485[i] >= 0) close(Run.rs485[i]);
    }
}

//	This is the main... 
int main(int argc, char **argv)
{    
    int i;
    for(;;) {
	InitDb();
	if (ReadConfig((argc > 1) ? argv[1] : "/home/daq/bin/pp2pp-slow.conf") < 0) return 100;
	if (Run.log) {
	    fprintf(Run.log, "**** Starting the server. ");
	    fflush(Run.log);
	}
    	for (i=0; i<2; i++) APCRead(i);	// We need check APC before anything else !
	InitSerial();
	if (Run.log) {
	    fprintf(Run.log, "ADAM initialized. ");
	    fflush(Run.log);
	}
	InitGpib();
	if (Run.log) {
	    fprintf(Run.log, "GPIB initialized. \n");
	    fflush(Run.log);
	}
	SRSCheck();		// we need read SRSes to understand their status.
	OneInit();		// understand real one button status
	if (InitUdp() < 0) return 200;
	if (Run.log) {
	    fprintf(Run.log, "Init done...\n");
	    fflush(Run.log);
	}
	for (;;) {
	    if (OneStep()) {
	        if (Run.log) DoLog(Run.log);
	        CheckRS485();
	    }
	    if (ServUdp()) break;
	}
	if (Run.log) {
	    fprintf(Run.log, "**** Stopping the server...\n");
	    fflush(Run.log);
	}
	CloseAll();
    }
    return 0;
}

