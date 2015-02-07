#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>
// #include <pthread.h>
#include "pp2pp-slow.h"

// static int Udp;
DevStatus Udp_Status;
static DbStruct Db;
static double vset[4];	// internal memory for Vset
// pthread_mutex_t Mutex;

// void pps_UDP_Close(void) {
//
//    if (Udp < 0) return; 
//    close(Udp);
//    Udp = -1;
//    Udp_Status = ST_OFF;
//    return;
//}

int pps_UDP_Init(void) {

    int irc;
//    struct sockaddr_in addr;
    
//    if (Udp >= 0) pps_UDP_Close();
//    pthread_mutex_init(&Mutex, NULL);
    
//    Udp = socket(PF_INET, SOCK_DGRAM, 0);
//    if (Udp < 0) {
//        return -1;
//    }
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(PPS_UDP_PORT);
//    inet_aton(PPS_UDP_SERVER, &addr.sin_addr);
//    irc = connect(Udp, (struct sockaddr *) &addr, sizeof(addr));
//    if (irc < 0) {
//        printf("FATAL - Can not connect: %m.\n");
//        pps_UDP_Close();
//        return -2;
//    }
    // initialize database
    memset(&Db, 0, sizeof(Db));
    memset(vset, 0, sizeof(vset));
    Udp_Status = ST_ON;
    return 0;
}

int pps_UDP_Operate(UdpIn * cmd, char * buf, int size) {

    int irc;
    fd_set set;
    struct timeval tout;
    int Udp;
    struct sockaddr_in addr;

    Udp = socket(PF_INET, SOCK_DGRAM, 0);
    if (Udp < 0) {
	Udp_Status = ST_NOCOM;
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PPS_UDP_PORT);
    inet_aton(PPS_UDP_SERVER, &addr.sin_addr);
//    irc = connect(Udp, (struct sockaddr *) &addr, sizeof(addr));
//    if (irc < 0) {
//        printf("FATAL - Can not connect: %m.\n");
//        pps_UDP_Close();
//        return -2;
//    }
//    if (Udp < 0) return -3; 
//    pthread_mutex_lock(&Mutex);
//    irc = write(Udp, cmd, sizeof(UdpIn));
    irc = sendto(Udp, cmd, sizeof(UdpIn), 0, (struct sockaddr *) &addr, sizeof(addr));
    // we actually never have an error here
    if (irc <= 0) {
	Udp_Status = ST_NOCOM;
//	pthread_mutex_unlock(&Mutex);
	return irc;
    }

    FD_ZERO(&set);
    FD_SET(Udp, &set);
    tout.tv_sec = 0;
    tout.tv_usec = PPS_UDP_MSTOUT*3000;
    while ((( irc = select(FD_SETSIZE, &set, NULL, NULL, &tout)) < 0) && (errno == EINTR)) ;
    // 0 means timeout (bad command or real timeout because of server problems)
    // <0 is real error, should never happen
    if (irc == 0) {
	Udp_Status = ST_NOCOM;
//	pthread_mutex_unlock(&Mutex);
	return irc;
    }
    if (irc < 0) {
	Udp_Status = ST_UNREL;
//	pthread_mutex_unlock(&Mutex);
	return irc;
    }
    irc = read(Udp, buf, size);
//    pthread_mutex_unlock(&Mutex);
    // when serever is not running we get -1 here (connection refused)
    if (irc <= 0) {
	Udp_Status = ST_NOCOM;
	return irc;
    }
    close(Udp);
    Udp_Status = ST_ON;
    return irc;
}

// Checks the connection to the slow server
// returns the status of the connection
int pps_GetEcho(void) {

    UdpIn cmd;
    int buf[20];
    int irc;
    
    // Send echo command, should return the same
    cmd.cmd = cmd_echo;
    cmd.par[0] = rand();
    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
    if (irc <= 0) return Udp_Status;
    if ((buf[0] != cmd.cmd) || (buf[1] != cmd.par[0])) Udp_Status = ST_UNREL;
    return Udp_Status;
}

// Gets the main database structure from the srver
// and returns in the format, acceptable for Labview
// stores the database in internal variable for pps_GetVoltages access
// returns the status of the connection to the server
int pps_GetData(
	int * Alarm,		// 1 Grand total OR of all alarm states
	int * EStatus,		// 6 EAST adam modules status:
				// 4570 Enet, 5000, slot0-3
	int * WStatus,		// 6 WEST adam modules status:
				// 4570 Enet, 5000, slot0-3
	int * Stsrs, 		// 4 SRS status words
	double * Vsrs, 		// 4 SRS actual voltages, V
	double * Isrs, 		// 4 SRS actual bias currents, uA
	int * Stboard, 		// 32 board status words
	double * Ibias, 	// 32 detector bias currents, uA
	double * Temp,		// 32 detector temperature reading, F or C, dependent on TempU
	double * Vset,		// 4 SRS setting voltages, V
	int * vchange,		// 0 if no changes of Vset from db were detected, nonzero otherwise
	double * progress	// 0 to 1 aquire loop progress indicator
	) 		
{
    UdpIn cmd;
    int i, j, irc;
    DevStatus Stcomm[2];

    // Send getdb command
    cmd.cmd = cmd_getdb;
    irc = pps_UDP_Operate(&cmd, (char *)&Db, sizeof(Db));
    if (irc <= 0) {
	*Alarm = ST_NOCOM;
	return Udp_Status;
    }
    // Means wrong version of slow server
    if (Db.len != sizeof(Db)) {
	Udp_Status = ST_ALARM;
	*Alarm = ST_ALARM;
	return Udp_Status;
    }
    // Grand total Alarm
    *Alarm = ST_OFF;
    for (i=0; i<4; i++) if (Db.srs[i].status == ST_ALARM) *Alarm = ST_ALARM;
    for (i=0; i<32; i++) if (Db.si[i].status == ST_ALARM) *Alarm = ST_ALARM;
    for (i=0; i<4; i++) if (Db.srs[i].status == ST_TRIP) *Alarm = ST_TRIP;
    // 4570 communication satus
    // only can suupect no communication if all boards and adams do not reply
    for (j=0; j<2; j++) {
	Stcomm[j] = ST_NOCOM;
	if (Db.adam5000E[j] != ST_NOCOM && Db.adam5000E[j] != ST_POFF) {
	    Stcomm[j] = ST_ON;
	    continue;
	}
    }
    // Adam 4570 status
    EStatus[0] = Stcomm[0];
    WStatus[0] = Stcomm[1];
    // adam controllers status
    EStatus[1] = Db.adam5000E[0];
    WStatus[1] = Db.adam5000E[1];
    // adam modules status
    for (i=0; i<4; i++) EStatus[2+i] = Db.adam5017[0][i];
    for (i=0; i<4; i++) WStatus[2+i] = Db.adam5017[1][i];
    // SRS status
    for (i=0; i<4; i++) Stsrs[i] = Db.srs[i].status;
    // SRS current voltage
    for (i=0; i<4; i++) Vsrs[i] = Db.srs[i].vout;
    // SRS total bias current
    for (i=0; i<4; i++) Isrs[i] = Db.srs[i].iout;
    // board status
    for (i=0; i<32; i++) Stboard[i] = Db.si[i].status;
    // bias currents
    for (i=0; i<32; i++) Ibias[i] = Db.si[i].current;
    // temperatures
    for (i=0; i<32; i++) Temp[i] = Db.si[i].temp;
    // SRS intended voltage
    *vchange = 0;
    for (i=0; i<4; i++) {
	if (fabs(Db.srs[i].vtarg - vset[i]) > 0.1) {
	    *vchange = 1;
	}
    }
    if (*vchange) {
	for (i=0; i<4; i++)
	    Vset[i] = vset[i] = Db.srs[i].vtarg;
    }
    *progress = (double)(Db.step + 1)/Db.maxstep;
    return Udp_Status;
}

// returns the board voltages previously stored by the last pps_GetData
int pps_GetVoltages(
	int chan,		// logical board number, 0-31
	char * name,		// 5 board name, string
	int * StBoard,		// 1 Board staus
	double * Volts,		// 4 board voltages AVDD1, AVDD2, DVDD, DPECL
	double * temp,
	double * curr)
{
//    const char names[4][3] = {"HI", "HO", "VD", "VU"};
    const char names[4][3] = {"1D", "1U", "2D", "2U"};

    *StBoard = Db.si[chan].status;
    Volts[0] = Db.si[chan].AVDD1;
    Volts[1] = Db.si[chan].AVDD2;
    Volts[2] = Db.si[chan].DVDD;
    Volts[3] = Db.si[chan].DPECL;
    *temp = Db.si[chan].temp;
    *curr = Db.si[chan].current;
    sprintf(name, "%c%s%c", ((chan & 0x10) ? 'W' : 'E'), names[(chan >> 2) & 3],
    'A'+(chan & 3));
    return Udp_Status;
}

// To adjust certain low voltage on a certain board
// returns the status of the connection
int pps_LVAdjust(
	int board, 	// board logical number 0-31
	int voltchan,	// 0-AVDD1, 1-AVDD2, 2-DVDD, 3-DPECL
	int incdec)	// >0-increment, <0-decrement 
{

    UdpIn cmd;
    int buf[20];
    int irc;
    
    // Send brd_adjust command
    cmd.cmd = brd_adjust;
    cmd.par[0] = board;
    cmd.par[1] = voltchan;
    cmd.par[2] = incdec;
    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
    if (irc <= 0) return Udp_Status;
    if (buf[0] != 0) Udp_Status = ST_UNREL;
    return Udp_Status;
}

// To switch certain low voltage board on/off
// returns the status of the connection
int pps_LVSwitch(
	int board, 	// board logical number 0-31
	int onoff)	// 0-off, !=0-on 
{

    UdpIn cmd;
    int buf[20];
    int irc;
    
    // Send brd_switch command
    cmd.cmd = brd_switch;
    cmd.par[0] = board;
    cmd.par[1] = onoff;
    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
    if (irc <= 0) return Udp_Status;
    if (buf[0] != 0) Udp_Status = ST_UNREL;
    return Udp_Status;
}


// To switch certain SRS immediately off
// returns the status of the connection
int pps_SRSOff(
	int SRSm) 	// SRS mask, logical number 0-3
{

    UdpIn cmd;
    int buf[20];
    int i, mask, irc;
    
    mask = SRSm;
    // Send srs_switch command
    cmd.cmd = srs_switch;
    cmd.par[1] = 0;
    for (i=0; i<4; i++) {
	if (mask & 1) {    
	    cmd.par[0] = i;
	    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
	    if (irc <= 0) return Udp_Status;
	    if (buf[0] != 0) Udp_Status = ST_UNREL;
	}
	mask = (mask >> 1);
    }
    return Udp_Status;
}

// To start certain SRS for ramping up to the preset target voltage
// returns the status of the connection
int pps_SRSOn(
	int SRSm) 	// SRS mask, logical number 0-3
{

    UdpIn cmd;
    int buf[20];
    int i, mask, irc;
    
    mask = SRSm;
    // Send srs_switch command
    cmd.cmd = srs_switch;
    cmd.par[1] = 1;
    for (i=0; i<4; i++) {
	if (mask & 1) {    
	    cmd.par[0] = i;
	    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
	    if (irc <= 0) return Udp_Status;
	    if (buf[0] != 0) Udp_Status = ST_UNREL;
	}
	mask = (mask >> 1);
    }
    return Udp_Status;
}

// To set SRS intended voltage, either immediately or in server's internal value
// Sends the command if one of the voltages is different from previously obtained from server
// returns the status of the connection
int pps_SRSSet(
	double * volt) 	// 4 values of SRS intended voltages
{

    UdpIn cmd;
    int buf[20];
    int i, mask, irc;
    
    // Send srs_set command
    cmd.cmd = srs_set;
    for (i=0; i<4; i++) {
	if (fabs(volt[i] - vset[i]) > 0.1) {    
	    cmd.par[0] = i;
	    cmd.par[1] = volt[i];
	    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
	    if (irc <= 0) return Udp_Status;
	    if (buf[0] != 0) Udp_Status = ST_UNREL;
	    vset[i] = volt[i];
	}
    }
    return Udp_Status;
}


// To switch on single APC channel
// returns the result: 0/1 off/on, -1 - err
int pps_APCSwitch(
	int APC, 	// 0/1 - e/w
	int chan,	// 1-8
	int what)	// 0/1 - off/on
{

    UdpIn cmd;
    int buf[20];
    int i, mask, irc;
    
    if (APC < 0 || APC > 1 || chan < 1 || chan > 8) return -1;
    // Send apc_switch command
    cmd.cmd = apc_switch;
    cmd.par[0] = APC;
    cmd.par[1] = chan;
    cmd.par[2] = (what) ? 1 : 0;
    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
    if (irc <= 0) return -1;
    return buf[0];
}

// To perform one button switching
int pps_OneSwitch(
	int what)	// 0/1 - off/on
{
    UdpIn cmd;
    int buf[20];
    int irc;
    
    // Send apc_switch command
    cmd.cmd = one_switch;
    cmd.par[0] = (what) ? 1 : 0;
    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
    if (irc <= 0) return -1;
    return buf[0];
}

// To obtain one button status
int pps_OneStatus(
	char * msg)	// 0/1 - off/on
{
    UdpIn cmd;
    int irc;
    OneStatus one;
    
    // Send apc_switch command
    cmd.cmd = one_status;
    irc = pps_UDP_Operate(&cmd, (char *)&one, sizeof(one));
    if (irc <= 0) {
	return Udp_Status;
    }
    strncpy(msg, one.msg, sizeof(one.msg));
    return one.status;
}


// to restart the serever
int pps_Restart(void) {

    UdpIn cmd;
    int irc, buf[20];

    cmd.cmd = cmd_restart;
    irc = pps_UDP_Operate(&cmd, (char *)buf, sizeof(buf));
    if (irc <= 0) return Udp_Status;
    if (buf[0] != 0) Udp_Status = ST_UNREL;
    return Udp_Status;
}
