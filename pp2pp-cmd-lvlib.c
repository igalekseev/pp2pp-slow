/*  Igor Alekseev & Dmitry Svirida. ITEP. 2015  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include "pp2pp-slow.h"
#include "lv-pp2pp-slow.h"

void Usage(void)
{
    printf("\tpp2pp slow control command line tool\n");
    printf("Usage: pp2pp-cmd [command-line | @command-file | ?]\n");
    printf("command-line should be in quotes to be processed as a single argument.\n");
    printf("command-file can contain many lines. Commands can be separated by semicolons.\n");
    printf("? - prints this help.\n");
    printf("\tCommands (case insensitive): \n");
    printf("Adjust N VOLT INC|DEC - adjust voltage on power board N. VOLT=AVDD1, AVDD2, DVDD, DPECL. INC|DEC for increment/decrement;\n");
    printf("apc N K ON/OFF - Switch on/off APC unit N channel K;\n");
    printf("Echo - check connection to the server;\n");
    printf("hv N [ON|OFF|voltage] - show SRS N setting or switch SRS bias on/off, or set voltage\n");
    printf("lv N [ON|OFF] - show information on power board N. With ON|OFF - switch it on/off;\n");
    printf("Quit - quit;\n");
    printf("Restart - restart the server;\n");
    printf("Show - print all the standard information.\n");
}

int pps_status(int rc)
{
    if (rc == ST_ON) return 0;
    printf("Communication error.\n");
    return 50;
}

int process_cmd(char *cmd)
{
    char *tok;
    char *ptr;
    const char DELIM[] = " \t=:";
    const char vocabulary[][20] = {"?", "adjust", "echo", "hv", "lv", "restart", "show", "quit", "apc"};
    const char volt[4][10] = {"AVDD1", "AVDD2", "DVDD", "DPECL"};
    int i, j, k, n, irc;
    double val;
    int Alarm;		    // 1 Grand total OR of all alarm states
	int EStatus[6];		// 6 EAST adam modules status:
				        // 4570 Enet, 5000, slot0-3
	int WStatus[6];		// 6 WEST adam modules status:
				        // 4570 Enet, 5000, slot0-3
	int Stsrs[4]; 		// 4 SRS status words
	double Vsrs[4];		// 4 SRS actual voltages, V
	double Isrs[4];		// 4 SRS actual bias currents, uA
	int Stboard[32];	// 32 board status words
	double Ibias[32]; 	// 32 detector bias currents, uA
	double Temp[32];	// 32 detector temperature reading, F or C, dependent on TempU
	double Vset[4];		// 4 SRS setting voltages, V
	int vchange;	    // 0 if no changes of Vset from db were detected, nonzero otherwise
	
	char name[10];		// 5 board name, string
	double Volts[4];	// 4 board voltages AVDD1, AVDD2, DVDD, DPECL
    
    const char statusstr[][10] = {"NOCOM", "OFF", "TRIP", "ALARM", "ON", "UNREL", "POFF", "RAMP"};
    
    irc = 0;
    tok = strtok(cmd, DELIM);
    for (i = 0; i < sizeof(vocabulary) / sizeof(vocabulary[0]); i++) if (!strcasecmp(tok, vocabulary[i])) break;
    
    switch(i) {
    case 0:     // ?
        Usage();
        break;
    case 1:     // Adjust
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for adjust.\n");
            irc = 201;
            break;
        }
        n = strtol(tok, NULL, 0);
        if (n < 0 || n > 31) {
            printf("%d out of range. Board number must be 0 to 31.\n", n);
            irc = 202;
            break;
        }
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for adjust.\n");
            irc = 203;
            break;
        }
        for (j = 0; j < 4; j++) if (!strcasecmp(tok, volt[j])) break;
        if (j == 4) {
            printf("Unknown voltage %s for adjust.\n", tok);
            irc = 204;
            break;
        }
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for adjust.\n");
            irc = 205;
            break;
        }
        if (!strcasecmp(tok, "INC")) {
            k = 1;
        } else if (!strcasecmp(tok, "DEC")) {
            k = -1;
        } else {
            printf("Unknown action for adjust: %s.\n");
            irc = 206;
            break;
        }
        irc = pps_status(pps_LVAdjust(n, j, k));
        break;
    case 2:     // Echo
        irc = pps_status(pps_LVAdjust(n, j, k));
        break;
    case 3:     // hv N [ON|OFF|voltage]
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for hv.\n");
            irc = 211;
            break;
        }
        n = strtol(tok, NULL, 0);
        if (n < 0 || n > 3) {
            printf("%d out of range. SRS number must be 0 to 3.\n", n);
            irc = 212;
            break;
        }
        tok = strtok(NULL, DELIM);
        if (!tok) {
            irc = pps_status(pps_GetData(&Alarm, EStatus, WStatus, Stsrs, Vsrs, Isrs, Stboard, Ibias, Temp,	Vset, &vchange));
            if (!irc) printf("SRS unit %d: Status = %s V = %6.1f I = %6.1f VSET = %6.1f\n", n, statusstr[Stsrs[n]], Vsrs[n], 1000000*Isrs[n], Vset[n]);
        } else if (!strcasecmp(tok, "ON")) {
            irc = pps_status(pps_SRSOn(1 << n)); 
        } else if (!strcasecmp(tok, "OFF")) {
            irc = pps_status(pps_SRSOff(1 << n)); 
        } else {
            val = strtod(tok, &ptr);
            if (ptr[0]) {
                printf("Bad voltage number: %s\n", tok);
                irc = 213;
                break;
            }
            irc = pps_status(pps_GetData(&Alarm, EStatus, WStatus, Stsrs, Vsrs, Isrs, Stboard, Ibias, Temp,	Vset, &vchange));
            if (!irc) {
                Vset[n] = val;
                irc = pps_status(pps_SRSSet(Vset));
            }
        }
        break;
    case 4:     // lv N [ON|OFF]
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for lv.\n");
            irc = 221;
            break;
        }
        n = strtol(tok, NULL, 0);
        if (n < 0 || n > 31) {
            printf("%d out of range. Board number must be 0 to 31.\n", n);
            irc = 222;
            break;
        }
        tok = strtok(NULL, DELIM);
        if (!tok) {
            memset(name, 0, sizeof(name));
            irc = pps_status(pps_GetVoltages(n, name, &Stboard[n], Volts));
            if (!irc) printf("LV board %s [%d]: Status = %s AVDD1 = %6.2f AVDD2 = %6.2f DVDD = %6.2f DPECL = %6.2f\n",
                 name, n, statusstr[Stboard[n]], Volts[0], Volts[1], Volts[2], Volts[3]);
        } else if (!strcasecmp(tok, "ON")) {
            irc = pps_status(pps_LVSwitch(n, 1)); 
        } else if (!strcasecmp(tok, "OFF")) {
            irc = pps_status(pps_LVSwitch(n, 0)); 
        } else {
            printf("Bad lv function: %s\n", tok);
            irc = 223;
        }
        break;
    case 5:     // restart
        irc = pps_status(pps_Restart());        
        break;
    case 6:     // show
        irc = pps_status(pps_GetData(&Alarm, EStatus, WStatus, Stsrs, Vsrs, Isrs, Stboard, Ibias, Temp,	Vset, &vchange));
        if (!irc) {
            printf("Global alarm = %s\n", statusstr[Alarm]);
            printf("ADAM East status = %s\t%s\t%s\t%s\t%s\t%s\n", 
                statusstr[EStatus[0]], statusstr[EStatus[1]], statusstr[EStatus[2]], statusstr[EStatus[3]], statusstr[EStatus[4]], statusstr[EStatus[5]]);
            printf("ADAM West status = %s\t%s\t%s\t%s\t%s\t%s\n", 
                statusstr[WStatus[0]], statusstr[WStatus[1]], statusstr[WStatus[2]], statusstr[WStatus[3]], statusstr[WStatus[4]], statusstr[WStatus[5]]);
            for (n=0; n<4; n++) printf("SRS unit %d: Status = %s V = %6.1f I = %6.1f VSET = %6.1f\n", n, statusstr[Stsrs[n]], Vsrs[n], 1000000*Isrs[n], Vset[n]);
            for (i=0; i<4; i++) {
                printf("LV boards %2d - %2d:", 8*i, 8*i + 7);
                for (j=0; j<8; j++) printf("%s ", statusstr[Stboard[8*i + j]]);
                printf("\n");
            }
            for (i=0; i<4; i++) {
                printf("Ibias %2d - %2d:", 8*i, 8*i + 7);
                for (j=0; j<8; j++) printf("%6.1f ", Ibias[8*i + j]);
                printf("\n");
            }
            for (i=0; i<4; i++) {
                printf("Temp %2d - %2d:", 8*i, 8*i + 7);
                for (j=0; j<8; j++) printf("%6.1fF ", Temp[8*i + j]);
                printf("\n");
            }
        }
        break;
    case 7:     // quit
        irc = 10000;
        break;
    case 8:	// apc N k ON/OFF - Switch on/off APC channel k
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for apc.\n");
            irc = 231;
            break;
        }
        n = strtol(tok, NULL, 0);
        if (n < 0 || n > 1) {
            printf("%d out of range. APC number must be 0 or 1.\n", n);
            irc = 232;
            break;
        }
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for apc.\n");
            irc = 233;
            break;
        }
	j = strtol(tok, NULL, 0);
        if (j < 0 || j > 7) {
            printf("%d out of range. APC channel number must be 0 to 7.\n", j);
            irc = 234;
            break;
        }
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for apc.\n");
            irc = 235;
            break;
        }
        if (!strcasecmp(tok, "ON")) {
            k = 1;
        } else if (!strcasecmp(tok, "OFF")) {
            k = 0;
        } else {
            printf("Unknown action for apc: %s.\n");
            irc = 236;
            break;
        }
        irc = pps_status(pps_APCSwitch(n, j, k));
	break;
    default:
        printf("Unknown command %s\n", tok);
        irc = 200;
    }    
    return irc;
}

int process_line(char *line)
{
    char *ptr;
    char *ln;
    char cmd[1024];
    int irc;
    
    irc = 0;
    ln = line;
    for (;;) {
        if (!ln[0]) break;
        ptr = strchr(ln, ';');
        if (ptr) {
            if (ptr - ln > sizeof(cmd) - 1) ptr = ln + sizeof(cmd) - 1;
            memcpy(cmd, ln, ptr - ln);
            cmd[ptr - ln] = '\0';
            ln = ptr + 1;
        } else {
            strncpy(cmd, ln, sizeof(cmd));
            ln += strlen(ln);
        }
        irc = process_cmd(cmd);
        if (irc == 10000) return irc;     // Quit        
    }
    return irc;
}

int process_file(char *name)
{
    FILE *f;
    int irc;
    char line[1024];
    
    irc = 0;
    f = fopen(name, "rt");
    if (!f) {
        printf("Can not open file %s\n", name);
        irc = 100;
        return irc;
    }
    for (;;) {
        if (!fgets(line, sizeof(line), f)) break;
        irc = process_line(line);
        if (irc == 10000) return irc;     // Quit
    }
    fclose(f);
    return irc;
}

int main(int argc, char **argv)
{
    char *line;
    int irc;
    
    line = NULL;
    irc = 0;
    if (argc > 1) {
        
        switch (argv[1][0]) {
        case '?':
            Usage();
            break;
        case '@':
            irc = process_file(&argv[1][1]);
            break;
        default:
            irc = process_line(argv[1]);
        }
        if (irc == 10000) irc = 0;  // Quit
        return irc;
    }
    
    for(;;) {
        line = readline("? - help, Q - quit >");
        if (line && line[0]) add_history(line);
        if (!line || toupper(line[0]) == 'Q') break;
        irc = process_line(line);
        free(line);
    }
    if (line) free(line);
    return irc;
}