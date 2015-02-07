#ifndef PP2PP_SLOW_H
#define PP2PP_SLOW_H

#define PPS_UDP_SERVER	"130.199.90.19"
#define PPS_UDP_PORT	5629
#define PPS_UDP_MSTOUT	500


/*      Database definitions        */

typedef enum {
    ST_NOCOM,   // no communication
    ST_OFF,     // swiched off
    ST_TRIP,    // device tripped
    ST_ALARM,   // something out of range
    ST_ON,      // device is on
    ST_UNREL,   // unreliable communication
    ST_POFF,	// AC power off
    ST_RAMP     // ramping SRS up or down
} DevStatus;

//	SRS 325 status and data
typedef struct {
    DevStatus status;
    double vset;    // V, last set to SRS
    double vout;    // V, lsat reading
    double iout;    // uA, last reading
    double vtarg;   // V, intended target voltage
} SRS325;

//	Silicon planes
typedef struct {
    DevStatus status;
    double current; // uA
    double temp;    // F
    double AVDD1;   // V
    double AVDD2;   // V
    double DVDD;    // V
    double DPECL;   // V
} SILICON;

// APS outlets
typedef struct {
   int swmask;
   int delay[8];
} APC;

//	Measurement results database
typedef struct {
    int len;            //  Db length - for control
    SRS325 srs[4];		//	SRSdata
    SILICON si[32];	    //	Silicon planes and their power, bias current etc
    double adam[2][4][8]; // ADAM read values - to be repacked into si, V
    DevStatus adam5017[2][4];	//	ADAM ADC modules status
    DevStatus adam5000E[2];	//	ADAM controllers status
    APC apc[2];			// APC
    int step, maxstep;		// server cycle step and number of steps
} DbStruct;

/*      Udp commands                */
typedef enum {
    cmd_echo,       // nothing - return the command
    cmd_getdb,      // get database - return Db
                    // On all other commands return 4-bytes int status. 0 - OK, negative - error, positive - warning.
    brd_switch,     // switch on/off silicon power board. Parameter 0: board number. Parameter 1: "0" - off, any other - on.
    brd_adjust,     // chnage voltage on silicon power board. Parameter 0: board number. Parameter 1: "0" - AVDD1, "1" - AVDD2, 
                    //      "2" - DVDD, "3" - DPECL. Parameter 2: negative - decrement, positive increment. One step always.
    srs_switch,     // switch on/off bias. Parameter 0: SRS325 number. Parameter 1: "0" - off, any other - on.
    srs_set,	    // set internal value of SRS intended voltage or ramp to this voltage if ON
    apc_switch,	    // switch on/off APC channels. Only works if nobody is logged into APC through web.
    cmd_restart,    // close io and start again
    one_switch,	    // switch everything on/off
    one_status      // return status of one switch (on, off, ramp, alarm)
} UdpCmd;

//  Udp command block
typedef struct {
    UdpCmd cmd;     // the command
    int par[5];     // parameters
} UdpIn;

typedef struct {
    DevStatus	status;		// status of the one button operation
    char	msg[1024];	// message for the operator
} OneStatus;

#endif // PP2PP_SLOW_H

