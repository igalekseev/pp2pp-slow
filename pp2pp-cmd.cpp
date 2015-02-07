/*  Igor Alekseev & Dmitry Svirida. ITEP. 2015  */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "pp2pp-slow.h"

void Usage(void)
{
    printf("\tpp2pp slow control command line tool\n");
    printf("Usage: pp2pp-cmd [command-line | @command-file | ?]\n");
    printf("command-line should be in quotes to be processed as a single argument.\n");
    printf("command-file can contain many lines. Commands can be separated by semicolons.\n");
    printf("?/Help - prints this help.\n");
    printf("\tCommands (case insensitive): \n");
    printf("brd N [{VOLT INC|DEC} | {ON|OFF}] - adjust/show/switch voltage on power board N. VOLT=AVDD1, AVDD2, DVDD, DPECL. INC|DEC for increment/decrement;\n");
    printf("apc [N K ON|OFF] - Switch/show on/off APC unit N channel K;\n");
    printf("srs N [ON|OFF|voltage] - show SRS N setting or switch SRS bias on/off, or set voltage;\n");
    printf("one [ON|OFF] - show status/do one button operation;\n");
    printf("Quit - quit;\n");
    printf("Restart - restart the server;\n");
    printf("Show/Info - print all the standard information.\n");
}

int udpDo(UdpCmd cmd, void *buf = NULL, int blen = 0, int par0 = 0, int par1 = 0, int par2 = 0, int par3 = 0, int par4 = 0)
{
    int us;
    int irc;
    struct sockaddr_in addr;
    UdpIn pack;
    int sstat;

    // Open udp
    us = socket(PF_INET, SOCK_DGRAM, 0);
    if (us < 0) {
        printf("FATAL - Can not create socket: %m.\n");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PPS_UDP_PORT);
    inet_aton(PPS_UDP_SERVER, &addr.sin_addr);
    // perform operation
    pack.cmd = cmd;
    pack.par[0] = par0;
    pack.par[1] = par1;
    pack.par[2] = par2;
    pack.par[3] = par3;
    pack.par[4] = par4;
    irc = sendto(us, &pack, sizeof(pack), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (irc <= 0) {
	printf("Error sending to the server: %m\n");
	return -10;
    }
    if (buf && blen) {
	irc = read(us, buf, blen);
    } else {
	irc = read(us, &sstat, sizeof(sstat));
    }
    if (irc <= 0) printf("Error getting from server: %m\n");
    // close udp
    close(us);
    
    return (irc > 0) ? 0 : -20;
}

int process_cmd(char *cmd)
{
    char *tok;
    char *ptr;
    const char DELIM[] = " \t=:";
    const char vocabulary[][20] = {"?", "help", "quit", "info", "show", "restart", "brd", "srs", "apc", "one"};
    const char volt[][10] = {"AVDD1", "AVDD2", "DVDD", "DPECL", "ON", "OFF"};
    int i, j, k, n, irc;
    double val;
    DbStruct Db;
    OneStatus one;
    
    const char statusstr[][10] = {"NOCOM", "OFF", "TRIP", "ALARM", "ON", "UNREL", "POFF", "RAMP"};
    
    irc = 0;
    tok = strtok(cmd, DELIM);
    for (i = 0; i < sizeof(vocabulary) / sizeof(vocabulary[0]); i++) if (!strcasecmp(tok, vocabulary[i])) break;
    
    switch(i) {
    case 0:     // ?
    case 1:	// Help
        Usage();
        break;
    case 2:     // quit
        irc = 10000;
        break;
    case 3:	// info
    case 4:	// show
        irc = udpDo(cmd_getdb, &Db, sizeof(Db));
        if (!irc) {
	    printf("Db length = %d\n", Db.len);
            for (n=0; n<4; n++) printf("SRS unit %d: Status = %8s V = %6.1fV I = %6.1fua VSET = %6.1fV VTARG = %6.1fV\n", n, statusstr[Db.srs[n].status], 
		Db.srs[n].vout, Db.srs[n].iout,  Db.srs[n].vset, Db.srs[n].vtarg);
            for (i=0; i<4; i++) {
                printf("LV boards %2d - %2d:", 8*i, 8*i + 7);
                for (j=0; j<8; j++) printf("%8s ", statusstr[Db.si[8*i + j].status]);
                printf("\n");
            }
            for (i=0; i<4; i++) {
                printf("    Ibias %2d - %2d:", 8*i, 8*i + 7);
                for (j=0; j<8; j++) printf("%8.1f ", Db.si[8*i + j].current);
                printf("\n");
            }
            for (i=0; i<4; i++) {
                printf("     Temp %2d - %2d:", 8*i, 8*i + 7);
                for (j=0; j<8; j++) printf("%7.1fF ", Db.si[8*i + j].temp);
                printf("\n");
            }
            printf("ADAM East status = %s\t%s\t%s\t%s\t%s\n", 
                statusstr[Db.adam5000E[0]], statusstr[Db.adam5017[0][0]], statusstr[Db.adam5017[0][1]], statusstr[Db.adam5017[0][2]], statusstr[Db.adam5017[0][3]]);
            printf("ADAM West status = %s\t%s\t%s\t%s\t%s\n", 
                statusstr[Db.adam5000E[1]], statusstr[Db.adam5017[1][0]], statusstr[Db.adam5017[1][1]], statusstr[Db.adam5017[1][2]], statusstr[Db.adam5017[1][3]]);
	    printf("APC East:");
	    for (i=0; i<8; i++) printf("%3s ", (Db.apc[0].swmask & (1 << i)) ? "ON" : "OFF");
	    printf("APC West:");
	    for (i=0; i<8; i++) printf("%3s ", (Db.apc[1].swmask & (1 << i)) ? "ON" : "OFF");
	    printf("\n");
        }
        break;
    case 5:	// restart
	irc = udpDo(cmd_restart);
	break;
    case 6:	// brd N [{VOLT INC|DEC} | {ON|OFF}] - adjust/show/switch voltage on power board N. VOLT=AVDD1, AVDD2, DVDD, DPECL. INC|DEC for increment/decrement;\n");
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for brd.\n");
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
            irc = udpDo(cmd_getdb, &Db, sizeof(Db));
	    if (irc) break;
	    printf("LV board %d: Status = %8s AVDD1 = %6.2fV AVDD2 = %6.2fV DVDD = %6.2fV DPECL = %6.2fV\n",
                 n, statusstr[Db.si[n].status], Db.si[n].AVDD1, Db.si[n].AVDD2, Db.si[n].DVDD, Db.si[n].DPECL);
            break;
        }
        for (j = 0; j < 6; j++) if (!strcasecmp(tok, volt[j])) break;
	switch (j) {
	case 0:
	case 1:
	case 2:
	case 3:
            tok = strtok(NULL, DELIM);
            if (!tok) {
                printf("Not enough arguments for brd adjust.\n");
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
            irc = udpDo(brd_adjust, NULL, 0, n, j, k);
	    break;
	case 4:
            irc = udpDo(brd_switch, NULL, 0, n, 1);
	    break;
	case 5:
            irc = udpDo(brd_switch, NULL, 0, n, 0);
	    break;
	default:
            printf("Unknown request %s for brd.\n", tok);
            irc = 204;
	}
        break;
    case 7:     // srs N [ON|OFF|voltage] - show SRS N setting or switch SRS bias on/off, or set voltage
        tok = strtok(NULL, DELIM);
        if (!tok) {
            printf("Not enough arguments for srs.\n");
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
            irc = udpDo(cmd_getdb, &Db, sizeof(Db));
            if (!irc) printf("SRS unit %d: Status = %8s V = %6.1fV I = %6.1fua VSET = %6.1fV VTARG = %6.1fV\n", n, statusstr[Db.srs[n].status], 
		Db.srs[n].vout, Db.srs[n].iout,  Db.srs[n].vset, Db.srs[n].vtarg);
        } else if (!strcasecmp(tok, "ON")) {
            irc = udpDo(srs_switch, NULL, 0, n, 1);
        } else if (!strcasecmp(tok, "OFF")) {
            irc = udpDo(srs_switch, NULL, 0, n, 0);
        } else {
            val = strtod(tok, &ptr);
            if (ptr[0]) {
                printf("Bad voltage number: %s\n", tok);
                irc = 213;
                break;
            }
	    j = val;
            irc = udpDo(srs_set, NULL, 0, n, j);
        }
        break;
    case 8:	// apc [N K ON|OFF] - Switch/show on/off APC unit N channel K;
        tok = strtok(NULL, DELIM);
        if (!tok) {
            irc = udpDo(cmd_getdb, &Db, sizeof(Db));
            if (irc) break;
	    printf("APC East:");
	    for (i=0; i<8; i++) printf("%3s ", (Db.apc[0].swmask & (1 << i)) ? "ON" : "OFF");
	    printf("APC West:");
	    for (i=0; i<8; i++) printf("%3s ", (Db.apc[1].swmask & (1 << i)) ? "ON" : "OFF");
	    printf("\n");
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
        irc = udpDo(apc_switch, NULL, 0, n, j+1, k);
	break;
    case 9:		// one [ON|OFF]
        tok = strtok(NULL, DELIM);
	if (!tok) {
	    irc = udpDo(one_status, &one, sizeof(one));
	    if (!irc) printf("One button status is %s. Message:\n%s\n", statusstr[one.status], one.msg);
	} else if (!strcasecmp(tok, "ON")) {
            irc = udpDo(one_switch, NULL, 0, 1);
        } else if (!strcasecmp(tok, "OFF")) {
            irc = udpDo(one_switch, NULL, 0, 0);
        } else {
            printf("Unknown action for one: %s.\n");
            irc = 241;
        }
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
