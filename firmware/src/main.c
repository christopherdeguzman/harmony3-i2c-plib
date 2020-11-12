/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include <string.h>

#define SW1_PRESSED 0

#define EEPROM_CLIENT_ADDR  0x50
#define EEPROM_MEMORY_ADDR_H 0x00
#define EEPROM_MEMORY_ADDR_L 0x12
#define TX_DATA_LENGTH 7
#define RX_DATA_LENGTH 5
#define ACK_DATA_LENGTH 1

uint8_t txData[] = {
    EEPROM_MEMORY_ADDR_H,
    EEPROM_MEMORY_ADDR_L,
    'C', 'H', 'R', 'I', 'S'
};

uint8_t rxData[RX_DATA_LENGTH];

typedef enum {
    APP_STATE_EEPROM_WRITE,
    APP_STATE_EEPROM_READ,
    APP_STATE_EEPROM_WAIT_WRITE_COMPLETE,
    APP_STATE_EEPROM_CHECK_INTERNAL_WRITE_STATUS,
    APP_STATE_EEPROM_WAIT_READ_COMPLETE,
    APP_STATE_VERIFY,
    APP_STATE_IDLE,
    APP_STATE_XFER_SUCCESSFUL,
    APP_STATE_XFER_ERROR
} APP_STATES;

typedef enum {
    APP_TRANSFER_STATUS_IN_PROGRESS,
    APP_TRANSFER_STATUS_SUCCESS,
    APP_TRANSFER_STATUS_ERROR,
    APP_TRANSFER_STATUS_IDLE
} APP_TRANSFER_STATUS;




void APP_SW1_Callback(GPIO_PIN pin, uintptr_t context) {
    if (memcmp(&txData[2], &rxData[0], RX_DATA_LENGTH) != 0) {
        LED1_Set();
    } else {
        LED2_Set();
    }

    LED3_Set();
}

void APP_I2C_Callback(uintptr_t context) {
    APP_TRANSFER_STATUS* transferStatus = (APP_TRANSFER_STATUS*)context;
    
    if (I2C1_ErrorGet() == I2C_ERROR_NONE) {
        if (transferStatus) {
            *transferStatus = APP_TRANSFER_STATUS_SUCCESS;
        }
    } else {
        if (transferStatus) {
            *transferStatus = APP_TRANSFER_STATUS_ERROR;
        }
    }

}

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    APP_STATES state = APP_STATE_EEPROM_WRITE;
    volatile APP_TRANSFER_STATUS transferStatus = APP_TRANSFER_STATUS_ERROR;
    uint8_t ackData = 0;
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    GPIO_PinInterruptCallbackRegister(SW1_PIN, APP_SW1_Callback, 0);
    GPIO_PinInterruptEnable(SW1_PIN);
    
    I2C1_CallbackRegister(APP_I2C_Callback, (uintptr_t) &transferStatus);    
    
    
    while ( true )
    {
        switch (state) {
            case APP_STATE_EEPROM_WRITE:
                
                transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                I2C1_Write(EEPROM_CLIENT_ADDR, &txData[0], TX_DATA_LENGTH);
                
                state = APP_STATE_EEPROM_WAIT_WRITE_COMPLETE;
                break;
                
            case APP_STATE_EEPROM_WAIT_WRITE_COMPLETE:
                
                if (transferStatus == APP_TRANSFER_STATUS_SUCCESS) {
                    /* Read the status of internal write cycle */
                    transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                    I2C1_Write(EEPROM_CLIENT_ADDR, &ackData, ACK_DATA_LENGTH);
                    state = APP_STATE_EEPROM_CHECK_INTERNAL_WRITE_STATUS;
                } else if (transferStatus == APP_TRANSFER_STATUS_ERROR) {
                    state = APP_STATE_XFER_ERROR;
                }
                break;
                
            case APP_STATE_EEPROM_CHECK_INTERNAL_WRITE_STATUS:

                if (transferStatus == APP_TRANSFER_STATUS_SUCCESS) {
                    state = APP_STATE_EEPROM_READ;
                } else if (transferStatus == APP_TRANSFER_STATUS_ERROR) {
                    /* EEPROM's internal write cycle is not complete. Keep checking. */
                    transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                    I2C1_Write(EEPROM_CLIENT_ADDR, &ackData, ACK_DATA_LENGTH);
                }
                break;
                
            case APP_STATE_EEPROM_READ:
                
                I2C1_WriteRead(EEPROM_CLIENT_ADDR, &txData[0], 2, &rxData[0], RX_DATA_LENGTH);
                LED3_Set();
                state = APP_STATE_IDLE;
                break;
            default:
                break;
        }
    
        /* Maintain state machines of all polled MPLAB Harmony modules. */
//        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

