#ifndef LV_PP2PP_SLOW_H
#define LV_PP2PP_SLOW_H


void pps_UDP_Close(void);
int pps_UDP_Init(void);

// Checks the connection to the slow server
// returns the status of the connection
int pps_GetEcho(void);

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
	int * vchange		// 0 if no changes of Vset from db were detected, nonzero otherwise
); 		

// returns the board voltages previously stored by the last pps_GetData
int pps_GetVoltages(
	int chan,		// logical board number, 0-31
	char * name,		// 5 board name, string
	int * StBoard,		// 1 Board staus
	double * Volts);		// 4 board voltages AVDD1, AVDD2, DVDD, DPECL

int pps_LVAdjust(
	int board, 	// board logical number 0-31
	int voltchan,	// 0-AVDD1, 1-AVDD2, 2-DVDD, 3-DPECL
	int incdec);	// >0-increment, <0-decrement 

// To switch certain low voltage board on/off
// returns the status of the connection
int pps_LVSwitch(
	int board, 	// board logical number 0-31
	int onoff);	// 0-off, !=0-on 

// To switch certain SRS immediately off
// returns the status of the connection
int pps_SRSOff(int SRSm); 	// SRS mask, logical number 0-3

// To start certain SRS for ramping up to the preset target voltage
// returns the status of the connection
int pps_SRSOn(int SRSm); 	// SRS mask, logical number 0-3

// To set SRS intended voltage, either immediately or in server's internal value
// Sends the command if one of the voltages is different from previously obtained from server
// returns the status of the connection
int pps_SRSSet(double * volt); 	// 4 values of SRS intended voltages

// To switch on single APC channel
// returns the result: 0/1 off/on, -1 - err
int pps_APCSwitch(
	int APC, 	// 0/1 - e/w
	int chan,	// 1-8
	int what);	// 0/1 - off/on

// to restart the serever
int pps_Restart(void);

#endif /* LV_PP2PP_SLOW_H */

