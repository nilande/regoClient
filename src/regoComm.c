/*
 * regoComm.c
 *
 * Higher order communications library for the Rego637 heatpump controller
 */

#include <stdio.h> /* For printf() etc */
#include <time.h>  /* For time() */

#include <regoComm.h>
#include <regoSerialIO.h>

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define REG_TYPE_UNKNOWN	0x0		// Value yet to be determined
#define REG_TYPE_BOOL			0x1		// On = 1, Off = 0
#define REG_TYPE_INT			0x2		// Integer value
#define REG_TYPE_TEMP			0x3		// Temperature value in 0.1 degrees C
#define REG_TYPE_FRAC			0x4		// Numberic value in 0.1 fractions

#define REG_TYPE_MASK			0xf		// Mask for possible register types
#define REG_TYPE_GRAPHITE	0x10	// Flag for inclusion in Graphite output

#define GRAPHITE_PREFIX		"heatpump."		// Prefix for Graphite output

/*****************************************************************************
 * Shared variables
 *****************************************************************************/

int graphiteOutputFlag = 0;				// Flag set by '--graphite'
int ignoreChecksumsFlag = 0;			// Flag set by '--ignore-checksums'
int showPacketsFlag = 0;					// Flag set by ‘--show-packets’

typedef struct {
	uint16_t address;		// Register address
	char* name;					// Short name (for command line use, no spaces)
	char* description;	// Description
	int type;						// See #define REG_TYPE_*. Used to guide interpretation
} registerLookupTable;

registerLookupTable knownRegisters[] = {
	{0x0000,"setting_heat_curve","Inställning värmekurva",REG_TYPE_FRAC},
	{0x0001,"setting_heat_curve_adj","Inställning värmekurva justering",REG_TYPE_FRAC}, // TBV
	{0x0008,"setting_temp_adj-35","Kurvjustering vid -35 grader ute", REG_TYPE_TEMP},
	{0x000a,"setting_temp_adj-30","Kurvjustering vid -30 grader ute", REG_TYPE_TEMP},
	{0x000c,"setting_temp_adj-25","Kurvjustering vid -25 grader ute", REG_TYPE_TEMP},
	{0x000e,"setting_temp_adj-20","Kurvjustering vid -20 grader ute", REG_TYPE_TEMP},
	{0x0010,"setting_temp_adj-15","Kurvjustering vid -15 grader ute", REG_TYPE_TEMP},
	{0x0012,"setting_temp_adj-10","Kurvjustering vid -10 grader ute", REG_TYPE_TEMP},
	{0x0014,"setting_temp_adj-5","Kurvjustering vid -5 grader ute", REG_TYPE_TEMP},
	{0x0016,"setting_temp_adj-0","Kurvjustering vid 0 grader ute", REG_TYPE_TEMP},
	{0x0018,"setting_temp_adj+5","Kurvjustering vid +5 grader ute", REG_TYPE_TEMP},
	{0x001a,"setting_temp_adj+10","Kurvjustering vid +10 grader ute", REG_TYPE_TEMP},
	{0x001c,"setting_temp_adj+15","Kurvjustering vid +15 grader ute", REG_TYPE_TEMP},
	{0x001e,"setting_temp_adj+20","Kurvjustering vid +20 grader ute", REG_TYPE_TEMP},
	{0x0021,"setting_room_temp","Inställning rumstemperatur",REG_TYPE_TEMP},
	{0x0022,"setting_room_temp_effect","Inställning rumsgivarpåverkan",REG_TYPE_FRAC}, // TBV
	{0x002b,"control_gt3_target","Styrning GT3 målvärde",REG_TYPE_TEMP}, // TBV
	{0x006c,"control_add_heat","Styrning tilläggsvärme %",REG_TYPE_FRAC}, // TBV
	{0x006e,"control_gt1_target","Styrning GT1 målvärde",REG_TYPE_TEMP}, // TBV
	{0x006f,"control_gt1_on","Styrning GT1 tillslag",REG_TYPE_TEMP}, // TBV
	{0x0070,"control_gt1_off","Styrning GT1 frånslag",REG_TYPE_TEMP}, // TBV
	{0x0073,"control_gt3_on","Styrning GT3 tillslag",REG_TYPE_TEMP}, // TBV
	{0x0074,"control_gt3_off","Styrning GT3 frånslag",REG_TYPE_TEMP}, // TBV
	{0x01fd,"status.p3GroundLoopPump","Status köldbärarpump (P3)",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x01fe,"status.compressor","Status kompressor",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x01ff,"status.addHeatStage1","Status elpatron 3kW",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x0200,"status.addHeatStage2","Status elpatron 6kW",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x0203,"status.p1RadiatorPump","Status radiatorpump (P1)",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x0204,"status.p2HeatCarrierPump","Status värmebärarpump (P2)",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x0205,"status.vxvThreeWayValve","Status trevägsventil (VXV)",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x0206,"status.alarm","Status alarm",REG_TYPE_BOOL | REG_TYPE_GRAPHITE},
	{0x0209,"sensors.temperature.gt1RadiatorReturn","Retur radiator (GT1)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x020A,"sensors.temperature.gt2Outdoor","Utomhustemperatur (GT2)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x020D,"sensors.temperature.gt5Room","Rumstemperatur (GT5)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
  {0x020E,"sensors.temperature.gt6Compressor","Kompressortemperatur (GT6)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x020F,"sensors.temperature.gt8HeatFluidOut","Temp. värmebärare ut (GT8)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x0210,"sensors.temperature.gt9HeatFluidIn","Temp. värmebärare in (GT9)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x0211,"sensors.temperature.gt10ColdFluidIn","Temp. köldbärare in (GT10)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x0212,"sensors.temperature.gt11ColdFluidOut", "Temp. köldbärare ut (GT11)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE},
	{0x0213,"sensors.temperature.gt3HotWater","Temp. varmvatten (GT3)",REG_TYPE_TEMP | REG_TYPE_GRAPHITE}
};

/*****************************************************************************
 * Internal function declarations
 *****************************************************************************/

/*****************************************************************************
 * Functions
 *****************************************************************************/

/* --- Useful for translating between addresses, names and descriptions --- */

/*
 * Get register ID given a certain name
 */
int8_t getRegisterIdByName(char* name) {
	int8_t i, len;
	len = sizeof(knownRegisters)/sizeof(knownRegisters[0]);
	for (i = 0; i < len; i++) {
		if (strcmp(knownRegisters[i].name, name) == 0) return i;
	}
	return -1; // No match found
}

/*
 * Get register ID given a certain address
 */
int8_t getRegisterIdByAddress(uint16_t address) {
	int8_t i, len;
	len = sizeof(knownRegisters)/sizeof(knownRegisters[0]);
	for (i = 0; i < len; i++) {
		if (knownRegisters[i].address == address) return i;
	}
	return -1; // No match found
}

/*
 * Get register Address from the lookup table, given a specific ID
 */
uint16_t getRegisterAddressById(int8_t id) {
	return knownRegisters[id].address;
}

/*
 * Get register Description from the lookup table, given a specific ID
 */
char* getRegisterDescriptionById(int8_t id) {
	return knownRegisters[id].description;
}

/*
 * Get register Name from the lookup table, given a specific ID
 */
char* getRegisterNameById(int8_t id) {
	return knownRegisters[id].name;
}

/* --- Higher level communications functions towards heatpump --- */

/*
 * Query for an integer value from the heatpump
 * Note: For temperature sensors, the value is typically in 1/10 degrees
 */
int8_t queryRegister(uint16_t reg, int16_t* value) {
	int8_t retval;

	buildPacket(DEVICE_HEATPUMP, COMMAND_READ_SYS_REG, reg, 0);
	if (showPacketsFlag) { puts("Sending packet: "); prettyPrintPacket(); }
	sendPacket();
	receivePacket();
	if (showPacketsFlag) { puts("Received packet: "); prettyPrintPacket(); }

	return decodeIntPacket(value);
}

/*
 * Query for the display
 */
int8_t queryDisplay(char* text) {
	int8_t retval;
	uint8_t i, pos = 0, len;

	// Fetch all four rows
	for (i = 0; i < 4; i++) {
		buildPacket(DEVICE_HEATPUMP, COMMAND_READ_DISPLAY, i, 0);
		if (showPacketsFlag) { puts("Sending packet: "); prettyPrintPacket(); }
		sendPacket();
		receivePacket();
		if (showPacketsFlag) { puts("Received packet: "); prettyPrintPacket(); }
		// Decode onto the receive buffer, separate with the length of each line
		retval = decodeDisplayPacket(&len, text+pos);
		if (retval != RESPONSE_OK) return retval;
		pos += len-1; // -1 strips off null termination next loop
	}

	return retval;
}

/*
 * Wrapper around the queryRegister function that prints the results
 * 
 */
int8_t printRegister(uint16_t reg) {
	int8_t retval; /* Heatpump return value */
	int16_t value; /* Heatpump register value */

	/* Query register value from heatpump */
	retval = queryRegister(reg, &value);
				
	/* Check response */				
	if (retval != RESPONSE_OK) {
		/* Suppress errors for Graphite output */
		if (!graphiteOutputFlag) printf("Error %d requesting register %04x.\n", retval, reg);
		return retval;
	}

	/* Print info. Additional info if possible */
	int8_t id = getRegisterIdByAddress(reg);
	if (id < 0) {
		/* Suppress Graphite output for unknown registers */
		if (!graphiteOutputFlag) printf("%04x: %d\n", reg, value);
	} else if (!graphiteOutputFlag) {
		switch(knownRegisters[id].type & REG_TYPE_MASK) {
			case REG_TYPE_BOOL:
				printf("%s(%04x) - %s: %s\n", getRegisterNameById(id), reg,
	            	getRegisterDescriptionById(id), value ? "ON" : "OFF");
				break;
			case REG_TYPE_TEMP:
				printf("%s(%04x) - %s: %.1f degrees\n", getRegisterNameById(id), reg,
	            	getRegisterDescriptionById(id), (float) value / 10);
				break;
			case REG_TYPE_FRAC:
				printf("%s(%04x) - %s: %.1f\n", getRegisterNameById(id), reg,
	            	getRegisterDescriptionById(id), (float) value / 10);
				break;
			default:	
				printf("%s(%04x) - %s: %d\n", getRegisterNameById(id), reg,
	            	getRegisterDescriptionById(id), value);				
		}
	} else {
		switch(knownRegisters[id].type & REG_TYPE_MASK) {
			case REG_TYPE_TEMP:
			case REG_TYPE_FRAC:
				printf("%s%s %.1f %u\n", GRAPHITE_PREFIX, getRegisterNameById(id), (float) value / 10, (unsigned)time(NULL));
				break;
			default:
				printf("%s%s %d %u\n", GRAPHITE_PREFIX, getRegisterNameById(id), value, (unsigned)time(NULL));
		}
	}

	return RESPONSE_OK;
}

/*
 * Print all known registers
 */
void printKnownRegisters() {
	int8_t i, len;
	len = sizeof(knownRegisters)/sizeof(knownRegisters[0]);
	for (i = 0; i < len; i++) {
		if (!graphiteOutputFlag) {
			printRegister(knownRegisters[i].address);
		} else if (knownRegisters[i].type & REG_TYPE_GRAPHITE) {
			/* For Graphite output, only include registers with flag set */
			printRegister(knownRegisters[i].address);
		}
	}
}

