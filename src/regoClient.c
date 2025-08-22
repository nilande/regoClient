#include <errno.h>	// Used for perror(), etc
#include <fcntl.h>
#include <getopt.h>	// Used for getopt()
#include <stdio.h>	// Used for printf(), etc
#include <stdlib.h>	// Used for exit(), etc

#include <regoComm.h>

void printUsage(char* cmd) {
	printf("Usage: %s [options] command [arg] [command [arg] [...]\n"
	       "\nAvailable commands:\n"
	       "   read_register (address) - Displays the 16-bit int content of the register at\n"
	       "                             the specified address\n"
				 "      read_known_registers - Query and print all known registers\n"
				 "  read_reg_range (fr) (to) - Query and print all registers in the given range\n"
	       "              show_display - Displays the info currently on the LCD display\n"
	       "\nAvailable options:\n"
				 "         --graphite-output - Outputs data suitable for Graphite logging\n"
				 "        --ignore-checksums - Just prints a warning if checksum error occurs\n"
	       "            --show-packets - Prints packets sent and received in hex form\n"
	       "\nNotes:\n"
	       "- Addresses can be specified using their name or numeric address\n"
	       "- Numeric values need to be specified in a numeric format supported by strol(),\n"
	       "such as '1234', '0x020b', '0b1010', etc.\n", cmd);
}

int main (int argc, char **argv) {
  int c; /* Argument char */
	int8_t retval; /* Heatpump return value */

  while (1) {
    static struct option long_options[] = {
			{"graphite-output", no_argument, &graphiteOutputFlag, 1},
    	{"ignore-checksums", no_argument, &ignoreChecksumsFlag, 1},
    	{"show-packets", no_argument, &showPacketsFlag, 1},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "",
                     long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* A flag or long option without short equivalent */
      //if (long_options[option_index].flag != 0) break;
      break;

    case '?':
      /* getopt_long already printed an error message. */
      break;

    default:
      abort ();
    }
  }

  /* Parse the action commands following the options */
  if (optind == argc) {
    printUsage(argv[0]);
		exit(0);
  }

  openSerialPort();

	/*
	 * Main command interpreter loop - this is where the action happens!
   */
	while (optind < argc) {
    if (strcmp("show_display", argv[optind]) == 0) {

			/*
			 * Show display contents
			 */
			char text[170]; 			// 21 chars x 4 rows in UTF-8 = 170 bytes to be safe
			retval = queryDisplay(text);
                        if (retval < 0) {
                                printf("Error %d querying display.\n", retval);
                                break;
                        }
                        printf("%s", text);

		} else if (strcmp("read_register", argv[optind]) == 0) {
			/*
			 * Show register contents
			 */
			if (optind+1 == argc) {
				printf("Command %s requires a parameter.\n", argv[optind]);
				break;
			}
			optind++;

			/* Try to find register by name from lookup table. */
			int8_t id = getRegisterIdByName(argv[optind]);
			uint16_t reg;
			if (id < 0) {
				/* Not in lookup table, try instead to convert text to register address directly */					
				reg = strtol(argv[optind], NULL, 0); 
				if (reg == 0) {
					printf("Parameter %s for %s could not be interpreted.\n", argv[optind], argv[optind-1]);
					break;
				}
			} else {
				/* Get register from lookup table */
				reg = getRegisterAddressById(id);
			}

			/* Print register contents */
			printRegister(reg);

		} else if (strcmp("read_known_registers", argv[optind]) == 0) {

			/*
			 * Print a list of all registers and their contents
			 */
			printKnownRegisters();

		} else if (strcmp("read_reg_range", argv[optind]) == 0) {

			/*
			 * Show register range contents
			 */
			if (optind+2 >= argc) {
				printf("Command %s requires two parameters.\n", argv[optind]);
				break;
			}
			optind+=2;

			uint16_t reg, reg1, reg2;
			reg1 = strtol(argv[optind-1], NULL, 0);
			reg2 = strtol(argv[optind], NULL, 0);
			if ((reg1 >= reg2) || (reg1+0xff < reg2)) {
				printf("Register range for %s must be numbers, and between 1-256 addresses apart.\n", argv[optind-2]);
				break;
			}

			/* Print a list of all registers and their contents */
			for (reg = reg1; reg <= reg2; reg++) {
				printRegister(reg);
			}

		} else {

			printf("Invalid command %s.\n", argv[optind]);

		}

		optind++;
  }
	
  closeSerialPort();

	exit(0);
}

