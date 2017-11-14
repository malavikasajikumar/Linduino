#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "UserInterface.h"
#include "QuikEval_EEPROM.h"
#include <platform_drivers.h>
extern "C" {
    #include <ad5686.h>
};


/******************************************************************************/
/************************** Variables Declarations ****************************/
/******************************************************************************/

// Converts our DAC code into letters of which DAC is selected
const String dac_selection[16] = {
    "None",
    "A",
    "B",
    "AB",
    "C",
    "AC",
    "BC",
    "ABC",
    "D",
    "AD",
    "BD",
    "ABD",
    "CD",
    "ACD",
    "BCD",
    "All"
};

ad5686_init_param init_params = {
    GENERIC_I2C,    // I2C type
    0,              // I2C ID
    0,              // I2C max speed
    0,              // I2C slave address
    GENERIC_SPI,    // SPI type
    QUIKEVAL_CS,    // SPI id
    45000000,        // SPI max speed (hz)
    SPI_MODE_1,     // SPI mode
    QUIKEVAL_CS,    // SPI chipselect
    GENERIC_GPIO,   // GPIO type
    0,              // GPIO id
    4,              // GPIO reset pin
    5,              // GPIO LDAC pin
    ID_AD5686R,     // Device type
};

ad5686_dev * device;

void setup()
{
    char demo_name[] = "AD5686R";
    
    Serial.begin(115200);
    
    // Give the serial port a chance to initialize
    // Without this we get some garbage on the console
    delay(100);

    int32_t ret = ad5686_init(&device, init_params);

    Serial.print("Initialized, ");
    Serial.println(ret);

    // Set GAIN high
    gpio_set_value(&device->gpio_dev, 2, GPIO_HIGH);

    // Set LDAC high so we can write to registers without updating
    gpio_set_value(&device->gpio_dev, 5, GPIO_HIGH);
    
    print_title();
}

void loop()
{

    /*float fvoltage = 1.24;
    uint16_t voltage = voltage_to_code(fvoltage, 2.5);

    Serial.print(F("Converting "));
    Serial.print(fvoltage);
    Serial.print(F(" to "));
    Serial.println(voltage, HEX);

    ad5686_write_update_register(device, AD5686_CH_A, voltage);

    delay(900);

    //uint8_t result = ad5686_read_back_register(device, AD5686_CH_A);
    //Serial.print("Readback of register: ");
    //Serial.println(result, HEX);

    delay(100);*/
    //ad5686_write_update_register(device, AD5686_CH_ALL, 0xF0F0);
    //delay(1000);
    // DAC(s) to perform actions on
    // Binarily represents which are selected with bits arranged like so: DCBA
    // Examples:
    //   0110 = B & C
    //   1010 = B & D
    //   1101 = A, C & D
    //   1111 = All
    static int16_t selected_dac = 1;
    static float ref_voltage = 2.5;

    // Switch menu based on user's input
    print_prompt(selected_dac, ref_voltage);
    int user_command = read_int();
    Serial.println(user_command);
    Serial.flush();
    switch (user_command)
    {
    case 1:
        menu_1_select_dac(&selected_dac);
        break;
        
    case 2:
        menu_2_write_to_input_register(selected_dac, ref_voltage);
        break;
        
    case 3:
        menu_3_update_dac(selected_dac);
        break;
        
    case 4:
        menu_4_write_and_update_dac(selected_dac, ref_voltage);
        break;
        
    case 5:
        menu_5_power_up_down_DAC(selected_dac);
        break;
        /*
    case 6:
        menu_6_select_ref_voltage(&ref_voltage);
        break;
        
    case 7:
        menu_7_read_back_registers();
        break;
        
    case 8:
        menu_8_set_ldac_mask();
        break;
        
    case 9:
        menu_9_assert_ldac();
        break;
        
    case 10:
        menu_10_set_gain();
        break;
        
    case 11:
        menu_11_assert_soft_reset();
        break;
        
    case 12:
        menu_12_assert_hard_reset();
        break;
*/
    default:
        Serial.println("Incorrect Option");
        break;
    }
}

// Convert voltage float to code the DAC understands
uint16_t voltage_to_code(float voltage, float vRef)
{
    if(1 > 0) // Replace 1 with gain state
    {
        vRef *= 2;
    }
    
    uint32_t max_code = ((uint32_t)1 << 16)-1; //
    return (unsigned short)(voltage * (float)max_code / vRef); // 5 is vRef*2 because of GAIN
}


//! Prints the title block
void print_title()
{
    Serial.println(F("*****************************************************************"));
    Serial.println(F("* EVAL-5686RSDZ Demonstration Program                           *"));
    Serial.println(F("*                                                               *"));
    Serial.println(F("* This program demonstrates communication with the AD5686R      *"));
    Serial.println(F("* 16-Bit Quad Rail-to-Rail DAC with SPI Interface.              *"));
    Serial.println(F("*                                                               *"));
    Serial.println(F("* Set the baud rate to 115200 select the newline terminator.    *"));
    Serial.println(F("*****************************************************************"));
}

// Prints the "main menu" prompt to the console
void print_prompt(int16_t selected_dac, float ref_voltage) //!< this parameter is passed so that it can be printed at the bottom of the menu.
{
    Serial.println(F("\nCommand Summary:"));

    Serial.println(F("  1 -Select DAC"));
    Serial.println(F("  2 -Write to input register (no update)"));
    Serial.println(F("  3 -Update DAC from input"));
    Serial.println(F("  4 -Write and update DAC"));
    Serial.println(F("  5 -Power up/down DAC"));
    Serial.println(F("  6 -Select reference voltage"));
    Serial.println(F("  7 -Read back all registers"));
    Serial.println(F("  8 -Set LDAC# mask"));
    Serial.println(F("  9 -Assert LDAC#"));
    Serial.println(F("  10-Set gain"));
    Serial.println(F("  11-Assert Software Reset"));
    Serial.println(F("  12-Assert Hardware Reset"));
    Serial.println();
    
    Serial.print(F("  Selected DAC: "));
    if(selected_dac < 2) Serial.print(0); // Print leading zeroes
    if(selected_dac < 4) Serial.print(0);
    if(selected_dac < 8) Serial.print(0);
    Serial.print(selected_dac, BIN);
    Serial.print(F(" "));
    Serial.println(dac_selection[selected_dac]);
    
    Serial.print(F("  Reference Voltage: "));
    Serial.print(ref_voltage);
    Serial.println(F("V"));

    Serial.println();
    Serial.print(F("Enter a command: "));
}
 
// Menu for user to select a DAC
// @param selected_dac - Reference of DAC identifier
uint8_t menu_1_select_dac(int16_t *selected_dac)
{
    Serial.println(F("  DAC selections are represented in binary with"));
    Serial.println(F("  bits corresponding to: DCBA"));
    Serial.println(F("  (See datasheet for details and explanation)"));
    Serial.println(F("  For example:"));
    Serial.println(F("    B0001 - DAC A"));
    Serial.println(F("    B0010 - DAC B"));
    Serial.println(F("    B0100 - DAC C"));
    Serial.println(F("    B1000 - DAC D"));
    Serial.println(F("    B0101 - DAC A&C"));
    Serial.println(F("    B1111 - ALL"));
    Serial.print(F("  Enter any combination of DAC(s): "));
    
    *selected_dac = read_int();

    if (*selected_dac > 15)
        *selected_dac = 15; // If user enters and invalid option, default to ALL.
    if (*selected_dac < 0)
        *selected_dac = 0;
    
    Serial.println(*selected_dac, BIN);
    Serial.print(F("  You selected DAC(s): "));
    Serial.println(dac_selection[*selected_dac]);
    
    return (0); // Always returns success, consider adding a fail code later.
}

//! Menu 2: Write to input register only.  Does not update the output voltage.
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge.
uint8_t menu_2_write_to_input_register(int16_t selected_dac, float vref) //!< DAC to be updated. 0=A, 1=B, 2=All
{
    unsigned short vdata = get_voltage_code(vref);
    
    ad5686_write_register(device, selected_dac, vdata);

    return 0;
}

// Updates DAC outputs from its input registers
uint8_t menu_3_update_dac(int16_t selected_dac)
{
    //AD568X_WriteFunction(AD568X_CMD_UPDATE_DAC_N, selected_dac, 0);
    ad5686_update_register(device, selected_dac);
    Serial.println(F("  Updated DAC(s)"));
    
    return 0;
}
 
// Menu for user to change a DAC's output voltage
// @param selected_dac - Reference of DAC identifier to update,
// corresponding to 0=A, 1=B, 2=C, 3=D, 4=A&B, 5=ALL
uint8_t menu_4_write_and_update_dac(int16_t selected_dac, float vref) //!< DAC to be updated. 0=A, 1=B, 2=All
{
    unsigned short vdata = get_voltage_code(vref);

    ad5686_write_update_register(device, selected_dac, vdata);

    return (0);
}


uint8_t menu_5_power_up_down_DAC(int16_t selected_dac)
{
    // Cancel if no DAC selected
    if(selected_dac == 0)
    {
        Serial.println(F("  No DAC selected, no changes made"));
        return 0;
    }
    
    // Prompt for power mode
    Serial.println(F("\n  Power Modes:"));
    Serial.println(F("    1-Normal Operation"));
    Serial.println(F("    2-1kOhm to GND Power-Down"));
    Serial.println(F("    3-100kOhm to GND Power-Down"));
    Serial.println(F("    4-Three-State Power-Down"));
    Serial.println();
    Serial.print(F("  Select a power mode: "));
    
    // Get input
    uint8_t mode_input = read_int();
    uint8_t selected_mode = 0;
    
    // Validate input
    if(mode_input > 4) mode_input = 4;
    if(mode_input < 1) mode_input = 1;
    
    // Show validated input on console
    Serial.println(mode_input);
    
    // Select proper power mode
    switch(mode_input)
    {
        case 1:
            selected_mode = AD5686_PWRM_NORMAL;
            break;
        
        case 2:
            selected_mode = AD5686_PWRM_1K;
            break;
            
        case 3:
            selected_mode = AD5686_PWRM_100K;
            break;
            
        case 4:
            selected_mode = AD5686_PWRM_THREESTATE;
            break;
            
        default:
            break;
    }
    
    // Check bit-wise which DACs are selected
    uint8_t dac1 = (selected_dac >> 0) & 1;
    uint8_t dac2 = (selected_dac >> 1) & 1;
    uint8_t dac3 = (selected_dac >> 2) & 1;
    uint8_t dac4 = (selected_dac >> 3) & 1;
    
    // Apply power to selected DACS
    if(dac1)
    {
        Serial.println(F("  Applying power mode to DAC A..."));
        ad5686_power_mode(device, AD5686_CH_A, selected_mode);
    }
    if(dac2)
    {
        Serial.println(F("  Applying power mode to DAC B..."));
        ad5686_power_mode(device, AD5686_CH_B, selected_mode);
    }
    if(dac3)
    {
        Serial.println(F("  Applying power mode to DAC C..."));
        ad5686_power_mode(device, AD5686_CH_C, selected_mode);
    }
    if(dac4)
    {
        Serial.println(F("  Applying power mode to DAC D..."));
        ad5686_power_mode(device, AD5686_CH_D, selected_mode);
    }
    
    Serial.println(F("  Done!"));
    return 0;
}
/*
// Select reference voltage
uint8_t menu_6_select_ref_voltage(float * vref)
{
    // Prompt for internal or external
    Serial.println(F("  1-Internal (2.5v)"));
    Serial.println(F("  2-External"));
    Serial.print(F("  Select a reference voltage source: "));
    
    uint8_t vref_source = read_int();
    Serial.println(vref_source);
    
    float fvref = 0; // Custom vref
    
    switch(vref_source)
    {
        case 1:
            *vref = 2.5;
            break;
            
        case 2:
            // If external, prompt for exact vref
            Serial.println(F("  Enter selected external reference voltage: "));
            fvref = read_float();
            Serial.print(fvref);
            Serial.println(F("V"));
            *vref = fvref;
            break;
            
        default:
            Serial.println(F("  Incorrect entry"));
            break;
    }
    
    return 0;
}

// Reads back all DAC registers
uint8_t menu_7_read_back_registers()
{
    uint32_t reg1 = AD5686_ReadBackRegister(AD5686_CH_A);
    uint32_t reg2 = AD5686_ReadBackRegister(AD5686_CH_B);
    uint32_t reg3 = AD5686_ReadBackRegister(AD5686_CH_C);
    uint32_t reg4 = AD5686_ReadBackRegister(AD5686_CH_D);
    
    Serial.println(F("\n  All DAC register values:"));
    Serial.print(F("    DAC A - "));
    Serial.println(reg1, HEX);
    Serial.print(F("    DAC B - "));
    Serial.println(reg2, HEX);
    Serial.print(F("    DAC C - "));
    Serial.println(reg3, HEX);
    Serial.print(F("    DAC D - "));
    Serial.println(reg4, HEX);
    
    return 0;
}

uint8_t menu_8_set_ldac_mask()
{
    Serial.println(F("  Enter LDAC mask (for binary, prefix with a B)"));
    Serial.println(F("  Masked channels will not be updated when LDAC is asserted"));
    Serial.println(F("  Example: B0101 masks A & C"));
    Serial.print(F("  Mask: "));
    
    uint8_t mask = read_int();
    if(mask > 15) mask = 15; // Clamp at 1111
    Serial.println(mask, BIN);
    
    AD5686_LdacMask(mask);
    
    Serial.println(F("  Updated LDAC mask"));
}

uint8_t menu_9_assert_ldac()
{
    AD5686_LDAC_LOW;
    delay(0.1); // Wait just in case our clock speed is too fast (not likely)
    AD5686_LDAC_HIGH;
    Serial.println(F("  Asserted LDAC"));
    
    return 0;
}

uint8_t menu_10_set_gain()
{
    Serial.println(F("  GAIN options: "));
    Serial.println(F("    1-Low gain (1x)"));
    Serial.println(F("    2-High gain (2x)"));
    Serial.print(F("  Make selection: "));
    
    uint8_t selected_gain = read_int();
    if(selected_gain > 2) selected_gain = 2;
    if(selected_gain < 1) selected_gain = 1;
    Serial.println(selected_gain);
    
    switch(selected_gain)
    {
        case 1:
            AD5686_GAIN_LOW;
            Serial.println(F("  Setting gain low"));
            break;
            
        case 2:
            AD5686_GAIN_HIGH;
            Serial.println(F("  Setting gain high"));
            break;
            
        default:
            break;
    }
    
    return 0;
}

uint8_t menu_11_assert_soft_reset()
{
    Serial.println(F("  Performing software reset"));
    
    AD5686_SoftwareReset();
    
    return 0;
}

uint8_t menu_12_assert_hard_reset()
{
    Serial.println(F("  Performing hardware reset"));
    
    // Pull reset low then high
    AD5686_RESET_LOW;
    delay(0.1); // Wait just in case our clock speed is faster than 30ns (unlikely)
    AD5686_RESET_HIGH;
    
    return 0;
}

// Convert voltage float to code the DAC understands
uint16_t voltage_to_code(float voltage, float vRef)
{
    if(AD5686_GAIN_STATE > 0)
    {
        vRef *= 2;
    }
    
    uint32_t max_code = ((uint32_t)1 << 16)-1; //
    return (unsigned short)(voltage * (float)max_code / vRef); // 5 is vRef*2 because of GAIN
}*/

// Gets a voltage from the user and converts it to the code the DAC understands
uint16_t get_voltage_code(float vRef)
{
    return voltage_to_code(get_voltage_float(), vRef);
}

// Prompts user to enter a voltage
// @return - float of the voltage
float get_voltage_float() //@TODO the arguments don't do anything, should remove. !< lsb weight of the LTC2607 (possibly calibrated) !< offset of LTC2607 (possibly calibrated)
{
    float dac_voltage;

    Serial.print(F("  Enter Desired DAC output voltage: ")); //! Prompt user to enter a voltage
    dac_voltage = read_float();                            //! Read a float from the serial terminal
    Serial.print(dac_voltage);
    Serial.println(F(" V"));
    Serial.flush();
    return dac_voltage;
}