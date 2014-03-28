#include "CMUcam4.h"
#include "CMUcom4.h"
#include "drivers/mss_uart/mss_uart.h"

//cmucam4_instance_t cmucam4;

void CMUCam4_init ( cmucam4_instance_t *cmucam4, mss_uart_instance_t* uart )
{
	CMUcom4_init(&cmucam4->_com, uart);
	cmucam4->_state = DEACTIVATED;
}

/*******************************************************************************
* Helper Functions
*******************************************************************************/

int CMUCam4_getPixel( CMUcam4_image_data_t * pointer,
                      int row, int column)
{
    if((pointer==NULL)||
    (row<CMUCAM4_MIN_BINARY_ROW)||(CMUCAM4_MAX_BINARY_ROW<row)||
    (column<CMUCAM4_MIN_BINARY_COLUMN)||(CMUCAM4_MAX_BINARY_COLUMN<column))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return ((pointer->pixels[(row * CMUCAM4_ID_T_C) + (column / 8)]
    >> (7 - (column & 7))) & 1);
}

int CMUCam4_setPixel(CMUcam4_image_data_t * pointer,
                      int row, int column, int value)
{
    int bitIndex; int byteIndex;

    if((pointer==NULL)||
    (row<CMUCAM4_MIN_BINARY_ROW)||(CMUCAM4_MAX_BINARY_ROW<row)||
    (column<CMUCAM4_MIN_BINARY_COLUMN)||(CMUCAM4_MAX_BINARY_COLUMN<column))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    bitIndex = (7 - (column & 7));
    byteIndex = ((row * CMUCAM4_ID_T_C) + (column / 8));

    pointer->pixels[byteIndex] =
    (((~(1<<bitIndex))&(pointer->pixels[byteIndex]))|((value?1:0)<<bitIndex));

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_andPixels(CMUcam4_image_data_t * destination,
                       CMUcam4_image_data_t * source0,
                       CMUcam4_image_data_t * source1)
{
    size_t index;

    if((destination == NULL) || (source0 == NULL) || (source1 == NULL))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    for(index = 0; index < CMUCAM4_ID_T_LENGTH; index++)
    {
        destination->pixels[index] =
        (source0->pixels[index] & source1->pixels[index]);
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_orPixels(CMUcam4_image_data_t * destination,
                      CMUcam4_image_data_t * source0,
                      CMUcam4_image_data_t * source1)
{
    size_t index;

    if((destination == NULL) || (source0 == NULL) || (source1 == NULL))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    for(index = 0; index < CMUCAM4_ID_T_LENGTH; index++)
    {
        destination->pixels[index] =
        (source0->pixels[index] | source1->pixels[index]);
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_xorPixels(CMUcam4_image_data_t * destination,
                       CMUcam4_image_data_t * source0,
                       CMUcam4_image_data_t * source1)
{
    size_t index;

    if((destination == NULL) || (source0 == NULL) || (source1 == NULL))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    for(index = 0; index < CMUCAM4_ID_T_LENGTH; index++)
    {
        destination->pixels[index] =
        (source0->pixels[index] ^ source1->pixels[index]);
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_notPixels(CMUcam4_image_data_t * destination)
{
    size_t index;

    if(destination == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    for(index = 0; index < CMUCAM4_ID_T_LENGTH; index++)
    {
        destination->pixels[index] =
        (~destination->pixels[index]);
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_isReadOnly(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->readOnly == 'R');
}

int CMUCam4_isHidden(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->hidden == 'H');
}

int CMUCam4_isSystem(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->system == 'S');
}


int CMUCam4_isVolumeID(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->volumeID == 'V');
}

int CMUCam4_isDirectory(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->directory == 'D');
}

int CMUCam4_isArchive(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->archive == 'A');
}

/*******************************************************************************
* State Functions
*******************************************************************************/

int CMUCam4_begin(cmucam4_instance_t *cam)
{
    int errorValue; int retVal0; int retVal1; static int resetTries;

    resetTries = CMUCAM4_RESET_TRIES;
    errorValue = setjmp(cam->_env);

    if(resetTries-- <= 0)
    {
    	CMUcom4_end(&cam->_com);
        return errorValue;
    }

    // Try to reset at fast, medium, and slow baud rates.

    cam->_state = DEACTIVATED;
    _setReadTimeout(cam, CMUCAM4_RESET_TIMEOUT);

    if(errorValue)
    {
    	CMUcom4_end(&cam->_com);
    }

    CMUcom4_begin(&cam->_com);
    CMUcom4_write_byte(&cam->_com, (uint8_t) '\0');
    CMUcom4_write_byte(&cam->_com, (uint8_t) '\0');
    CMUcom4_write_byte(&cam->_com, (uint8_t) '\0');
    CMUcom4_write_str(&cam->_com, "\rRS\r");

    CMUcom4_end(&cam->_com);

    // XXX: might need to perform reset at CMUCOM4_MEDIUM_BAUD_RATE and CMUCOM4_SLOW_BAUD_RATE
    // TODO: see above


    // Get the firmware version.
    _waitForString(cam, "\rCMUcam4 v");
    _readText(cam);

    if(sscanf(cam->_resBuffer, "%1d.%2d ", &retVal0, &retVal1) != 2)
    {
        longjmp(cam->_env, CMUCAM4_UNEXPECTED_RESPONCE);
    }

    cam->_version = ((enum _CMUcam4_version) ((retVal0 * 100) + retVal1));

    switch(cam->_version)
    {
        case VERSION_100: break;
        case VERSION_101: break;
        case VERSION_102: break;
        case VERSION_103: break;

        default: longjmp(cam->_env, CMUCAM4_UNSUPPORTED_VERSION); break;
    }

    _waitForIdle(cam);

    // Adjust the baud rate.

    _setReadTimeout(cam, CMUCAM4_NON_FS_TIMEOUT);

    // Set baud rate to 250000
    CMUcom4_write_str(&cam->_com, "BM ");
    CMUcom4_write_str(&cam->_com, CMUCOM4_FAST_BR_STRING);
    CMUcom4_write_byte(&cam->_com, (uint8_t) '\r');
    _waitForResponce(cam);

    // Adjust the stop bits.
    CMUcom4_write_str(&cam->_com, "DM ");
    CMUcom4_write_str(&cam->_com, CMUCOM4_FAST_SB_STRING);
    CMUcom4_write_byte(&cam->_com, (uint8_t) '\r');
    _waitForResponce(cam);

    cam->_state = ACTIVATED;
    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_end(cmucam4_instance_t *cam)
{
    if(cam->_state == DEACTIVATED)
    {
        return CMUCAM4_NOT_ACTIVATED;
    }

    cam->_state = DEACTIVATED;
    CMUcom4_end(&cam->_com);

    return CMUCAM4_RETURN_SUCCESS;
}

/*******************************************************************************
* System Level Commands
*******************************************************************************/

int CMUCam4_getVersion(cmucam4_instance_t *cam)
{
    return (cam->_state == ACTIVATED) ? cam->_version : CMUCAM4_NOT_ACTIVATED;
}

int CMUCam4_resetSystem(cmucam4_instance_t *cam)
{
    return (cam->_state == ACTIVATED) ? CMUCam4_begin(cam) : CMUCAM4_NOT_ACTIVATED;
}

int CMUCam4_sleepDeeply(cmucam4_instance_t *cam)
{
    return _commandWrapper(cam, "SD\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_sleepLightly(cmucam4_instance_t *cam)
{
    return _commandWrapper(cam, "SL\r", CMUCAM4_NON_FS_TIMEOUT);
}

/*******************************************************************************
* Camera Module Commands
*******************************************************************************/

int CMUCam4_cameraBrightness(cmucam4_instance_t *cam, int brightness)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CB %d\r", brightness) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_cameraContrast(cmucam4_instance_t *cam, int contrast)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CC %d\r", contrast) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_cameraRegisterRead(cmucam4_instance_t *cam, int reg)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CR %d\r", reg) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _intCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_cameraRegisterWrite(cmucam4_instance_t *cam, int reg, int value, int mask)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CW %d %d %d\r", reg, value, mask) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Camera Sensor Auto Control Commands
*******************************************************************************/

int CMUCam4_autoGainControl(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "AG %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_autoWhiteBalance(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "AW %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Camera Format Commands
*******************************************************************************/

int CMUCam4_horizontalMirror(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "HM %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_verticalFlip(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "VF %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}


/*******************************************************************************
* Camera Effect Commands
*******************************************************************************/

int CMUCam4_blackAndWhiteMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "BW %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_negativeMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "NG %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Auxiliary I/O Commands
*******************************************************************************/

int CMUCam4_getButtonState(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GB\r", CMUCAM4_NON_FS_TIMEOUT);
}

long CMUCam4_getButtonDuration(cmucam4_instance_t *cam)
{
    int errorValue; int resultValue; long returnValue;

    if((errorValue = _commandWrapper(cam, "GD\r", CMUCAM4_NON_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);
    resultValue = (sscanf(cam->_resBuffer, "%ld ", &returnValue) == 1);

    _waitForIdle(cam);
    return resultValue ? returnValue : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_getButtonPressed(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GP\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_getButtonReleased(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GR\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_panInput(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "PI\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_panOutput(cmucam4_instance_t *cam, int direction, int output)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "PO %d %d\r", direction, output) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_tiltInput(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "TI\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_tiltOutput(cmucam4_instance_t *cam, int direction, int output)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "TO %d %d\r", direction, output) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_getInputs(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GI\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_setOutputs(cmucam4_instance_t *cam, int directions, int outputs)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "SO %d %d\r", directions, outputs) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_LEDOff(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "L0\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_LEDOn(cmucam4_instance_t *cam, long frequency)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "L1 %ld\r", frequency) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Servo Commands
*******************************************************************************/

int CMUCam4_getServoPosition(cmucam4_instance_t *cam, int servo)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "GS %d\r", servo) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _intCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_setServoPosition(cmucam4_instance_t *cam, int servo, int active, int pulseLength)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "SS %d %d %d\r", servo, active, pulseLength) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_automaticPan(cmucam4_instance_t *cam, int active, int reverse)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "AP %d %d\r", active, reverse) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_automaticTilt(cmucam4_instance_t *cam, int active, int reverse)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "AT %d %d\r", active, reverse) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_autoPanParameters(cmucam4_instance_t *cam, int proportionalGain, int derivativeGain)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "PP %d %d\r", proportionalGain, derivativeGain) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_autoTiltParameters(cmucam4_instance_t *cam, int proportionalGain, int derivativeGain)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "TP %d %d\r", proportionalGain, derivativeGain) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Television Commands
*******************************************************************************/

int CMUCam4_monitorOff(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "M0\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_monitorOn(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "M1\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_monitorFreeze(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "MF %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_monitorSignal(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "MS %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Color Tracking Commands
*******************************************************************************/

int CMUCam4_getTrackingParameters(cmucam4_instance_t *cam, CMUcam4_tracking_parameters_t * pointer)
{
    int errorValue; int resultValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _commandWrapper(cam, "GT\r", CMUCAM4_NON_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);
    resultValue = (sscanf(cam->_resBuffer, "%d %d %d %d %d %d ",
    &(pointer->redMin),
    &(pointer->redMax),
    &(pointer->greenMin),
    &(pointer->greenMax),
    &(pointer->blueMin),
    &(pointer->blueMax)) == 6);

    _waitForIdle(cam);
    return resultValue ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_getTrackingWindow(cmucam4_instance_t *cam, CMUcam4_tracking_window_t * pointer)
{
    int errorValue; int resultValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _commandWrapper(cam, "GW\r", CMUCAM4_NON_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);
    resultValue = (sscanf(cam->_resBuffer, "%d %d %d %d ",
    &(pointer->topLeftX),
    &(pointer->topLeftY),
    &(pointer->bottomRightX),
    &(pointer->bottomRightY)) == 4);

    _waitForIdle(cam);
    return resultValue ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_resetTrackingParameters(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "ST\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_setTrackingParameters(cmucam4_instance_t *cam,
								   int redMin, int redMax,
                                   int greenMin, int greenMax,
                                   int blueMin, int blueMax)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "ST %d %d %d %d %d %d\r",
    redMin, redMax, greenMin, greenMax, blueMin, blueMax)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_resetTrackingWindow(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "SW\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_setTrackingWindow(cmucam4_instance_t *cam, int topLeftX, int topLeftY,
                               int bottomRightX, int bottomRightY)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "SW %d %d %d %d\r",
    topLeftX, topLeftY, bottomRightX, bottomRightY)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_idleCamera(cmucam4_instance_t *cam)
{
    int errorValue; static int resetTries; char cmdBuffer[CMUCAM4_IC_LENGTH];

    if(cam->_state == DEACTIVATED)
    {
        return CMUCAM4_NOT_ACTIVATED;
    }

    if(snprintf(cmdBuffer, CMUCAM4_IC_LENGTH,
    CMUCAM4_IC_STRING, (cam->_version / 100), (cam->_version % 100))
    >= (int) CMUCAM4_IC_LENGTH)
    {
        return CMUCAM4_COMMAND_OVERFLOW;
    }

    resetTries = CMUCAM4_IDLE_TRIES;
    errorValue = setjmp(cam->_env);

    if(resetTries-- <= 0)
    {
        return errorValue;
    }

    _setReadTimeout(cam, CMUCAM4_IDLE_TIMEOUT);
    CMUcom4_write_byte(&cam->_com, (uint8_t)'\0');
    CMUcom4_write_byte(&cam->_com, (uint8_t)'\0');
    CMUcom4_write_byte(&cam->_com, (uint8_t)'\0');
    CMUcom4_write_str(&cam->_com, "\rGV\r");
    _waitForString(cam, cmdBuffer);

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_trackColor(cmucam4_instance_t *cam)
{
    return _commandWrapper(cam, "TC\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_setAndTrackColor(cmucam4_instance_t *cam,
						int redMin, int redMax,
                        int greenMin, int greenMax,
                        int blueMin, int blueMax)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "TC %d %d %d %d %d %d\r",
    redMin, redMax, greenMin, greenMax, blueMin, blueMax)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _commandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_trackWindow(cmucam4_instance_t *cam, int redRange, int greenRange, int blueRange)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "TW %d %d %d\r", redRange, greenRange, blueRange)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _commandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_getHistogram(cmucam4_instance_t *cam, int channel, int bins)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "GH %d %d\r", channel, bins)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _commandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_getMean(cmucam4_instance_t *cam)
{
    return _commandWrapper(cam, "GM\r", CMUCAM4_NON_FS_TIMEOUT);
}

int CMUCam4_getTypeFDataPacket(cmucam4_instance_t *cam, CMUcam4_image_data_t * pointer)
{
    int errorValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'F')))
    {
        return errorValue;
    }

    if(strcmp(cam->_resBuffer, "F ") != 0)
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _readBinary(cam, pointer->pixels, CMUCAM4_ID_T_LENGTH, CMUCAM4_ID_T_LENGTH, 0);

    return (_readWithTimeout(cam) == '\r')
    ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_getTypeHDataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_1_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_1_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_getTypeH2DataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_2_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_1_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_getTypeH4DataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_4_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_4_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_getTypeH8DataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_8_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_8_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_getTypeH16DataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_16_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_16_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_getTypeH32DataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_32_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_32_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_getTypeH64DataPacket(cmucam4_instance_t *cam, CMUcam4_histogram_data_64_t * pointer)
{
    int errorValue; char * buffer = (cam->_resBuffer + sizeof('H')); size_t counter;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'H')))
    {
        return errorValue;
    }

    for(counter = 0; counter < CMUCAM4_HD_64_T_LENGTH; counter++)
    {
        if((*buffer) == '\0')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        pointer->bins[counter] = ((uint8_t) strtol(buffer, &buffer, 10));
    }

    if((*buffer) != '\0')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    return CMUCAM4_RETURN_SUCCESS;
}


int CMUCam4_getTypeSDataPacket(cmucam4_instance_t *cam, CMUcam4_statistics_data_t * pointer)
{
    int errorValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'S')))
    {
        return errorValue;
    }

    return (sscanf(cam->_resBuffer,
    "S %d %d %d %d %d %d %d %d %d %d %d %d ",
    &(pointer->RMean),
    &(pointer->GMean),
    &(pointer->BMean),
    &(pointer->RMedian),
    &(pointer->GMedian),
    &(pointer->BMedian),
    &(pointer->RMode),
    &(pointer->GMode),
    &(pointer->BMode),
    &(pointer->RStDev),
    &(pointer->GStDev),
    &(pointer->BStDev)) == 12)
    ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_getTypeTDataPacket(cmucam4_instance_t *cam, CMUcam4_tracking_data_t * pointer)
{
    int errorValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _responceWrapper(cam, 'T')))
    {
        return errorValue;
    }

    return (sscanf(cam->_resBuffer,
    "T %d %d %d %d %d %d %d %d ",
    &(pointer->mx),
    &(pointer->my),
    &(pointer->x1),
    &(pointer->y1),
    &(pointer->x2),
    &(pointer->y2),
    &(pointer->pixels),
    &(pointer->confidence)) == 8)
    ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_pollMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "PM %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_lineMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "LM %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_switchingMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "SM %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_testMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "TM %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_colorTracking(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CT %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_histogramTracking(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "HT %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_invertedFilter(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "IF %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_noiseFilter(cmucam4_instance_t *cam, int threshold)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "NF %d\r", threshold) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* File System Commands
*******************************************************************************/

int CMUCam4_changeAttributes(cmucam4_instance_t *cam, const char * fileOrDirectoryPathName,
                              const char * attributes)
{
    if((fileOrDirectoryPathName == NULL) || (attributes == NULL))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CA \"%s\" \"%s\"\r", fileOrDirectoryPathName, attributes)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_changeDirectory(cmucam4_instance_t *cam, const char * directoryPathAndName)
{
    if(directoryPathAndName == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CD \"%s\"\r", directoryPathAndName)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_diskInformation(cmucam4_instance_t *cam, CMUcam4_disk_information_t * pointer)
{
    int errorValue; int resultValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _commandWrapper(cam, "DI\r", CMUCAM4_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);
    memset(pointer->volumeLabel, '\0', CMUCAM4_VL_LENGTH + 1);
    memset(pointer->fileSystemType, '\0', CMUCAM4_FST_LENGTH + 1);

    resultValue = (sscanf(cam->_resBuffer,
    "\"%" CMUCAM4_VL_LENGTH_STR "c\" "
    "\"%" CMUCAM4_FST_LENGTH_STR "c\" "
    "%lxh %lxh %lu %lu %lu %lu ",
    pointer->volumeLabel,
    pointer->fileSystemType,
    &(pointer->diskSignature),
    &(pointer->volumeIdentification),
    &(pointer->countOfDataSectors),
    &(pointer->bytesPerSector),
    &(pointer->sectorsPerCluster),
    &(pointer->countOfClusters)) == 8);

    _waitForIdle(cam);
    return resultValue ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_diskSpace(cmucam4_instance_t *cam, CMUcam4_disk_space_t * pointer)
{
    int errorValue; int resultValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _commandWrapper(cam, "DS\r", CMUCAM4_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);

    resultValue = (sscanf(cam->_resBuffer,
    "%lu %lu ",
    &(pointer->freeSectorCount),
    &(pointer->usedSectorCount)) == 2);

    _waitForIdle(cam);
    return resultValue ? CMUCAM4_RETURN_SUCCESS : CMUCAM4_UNEXPECTED_RESPONCE;
}

int CMUCam4_formatDisk(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "FM\r", CMUCAM4_FS_TIMEOUT);
}

long CMUCam4_listDirectory(cmucam4_instance_t *cam, CMUcam4_directory_entry_t * pointer,
                            size_t size, unsigned long offset)
{
    int errorValue; unsigned long directorySize;

    if((errorValue = _commandWrapper(cam, "LS\r", CMUCAM4_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    for(directorySize = 0; 1; directorySize++)
    {
        _receiveData(cam);

        if((*cam->_resBuffer) == ':')
        {
            break;
        }

        if((pointer != NULL) && (offset <= directorySize) &&
        ((directorySize - offset) < ((unsigned long) size)))
        {
            memset(pointer[directorySize - offset].name,
            '\0', CMUCAM4_NAME_LENGTH + 1);
            memset(pointer[directorySize - offset].attributes,
            '\0', CMUCAM4_ATTR_LENGTH + 1);

            if(sscanf(cam->_resBuffer,
            " \"%" CMUCAM4_NAME_LENGTH_STR "c\" "
            "%" CMUCAM4_ATTR_LENGTH_STR "c ",
            pointer[directorySize - offset].name,
            pointer[directorySize - offset].attributes) != 2)
            {
                return CMUCAM4_UNEXPECTED_RESPONCE;
            }

            pointer[directorySize - offset].size = 0;

            if(strchr(pointer[directorySize - offset].attributes, 'D') == NULL)
            {
                if(sscanf(cam->_resBuffer,
                " \"%*" CMUCAM4_NAME_LENGTH_STR "c\" "
                "%*" CMUCAM4_ATTR_LENGTH_STR "c "
                "%lu ",
                &(pointer[directorySize - offset].size)) != 1)
                {
                    return CMUCAM4_UNEXPECTED_RESPONCE;
                }
            }
        }
    }

    return (long) directorySize; // Will be between 0 and 65,536 entries.
}

int CMUCam4_makeDirectory(cmucam4_instance_t *cam, const char * directoryPathAndName)
{
    if(directoryPathAndName == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "MK \"%s\"\r", directoryPathAndName)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_moveEntry(cmucam4_instance_t *cam, const char * oldEntryPathAndName,
                       const char * newEntryPathAndName)
{
    if((oldEntryPathAndName == NULL) || (newEntryPathAndName == NULL))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "MV \"%s\" \"%s\"\r", oldEntryPathAndName, newEntryPathAndName)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_printLine(cmucam4_instance_t *cam, const char * filePathAndName, const char * textToAppend)
{
    if((filePathAndName == NULL) || (textToAppend == NULL))
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "PL \"%s\" \"%s\"\r", filePathAndName, textToAppend)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

long CMUCam4_filePrint(cmucam4_instance_t *cam, const char * filePathAndName, uint8_t * buffer,
                        size_t size, unsigned long offset)
{
    int errorValue; unsigned long fileSize;

    if(filePathAndName == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if(snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "PR \"%s\"\r", filePathAndName) >= CMUCAM4_CMD_BUFFER_SIZE)
    {
        return CMUCAM4_COMMAND_OVERFLOW;
    }

    if((errorValue = _commandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);

    if(sscanf(cam->_resBuffer, "%lu ", &fileSize) != 1)
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    _readBinary(cam, buffer, size, fileSize, offset);

    _waitForIdle(cam);
    return (long) fileSize; // Will be between 0 and 2,147,483,647 bytes.
}

int CMUCam4_removeEntry(cmucam4_instance_t *cam, const char * fileOrDirectoryPathAndName)
{
    if(fileOrDirectoryPathAndName == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "RM \"%s\"\r", fileOrDirectoryPathAndName)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_unmountDisk(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "UM\r", CMUCAM4_FS_TIMEOUT);
}

/*******************************************************************************
* Image Capture Commands
*******************************************************************************/

int CMUCam4_dumpBitmap(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "DB\r", CMUCAM4_FS_TIMEOUT);
}

int CMUCam4_dumpFrame(cmucam4_instance_t *cam, int horizontalResolution, int verticalResolution)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "DF %d %d\r", horizontalResolution, verticalResolution)
    < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int CMUCam4_sendBitmap(cmucam4_instance_t *cam, CMUcam4_image_data_t * pointer)
{
    int errorValue;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    if((errorValue = _commandWrapper(cam, "SB\r", CMUCAM4_NON_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _readBinary(cam, pointer->pixels, CMUCAM4_ID_T_LENGTH, CMUCAM4_ID_T_LENGTH, 0);

    if(_readWithTimeout(cam) != '\r')
    {
        return CMUCAM4_UNEXPECTED_RESPONCE;
    }

    _waitForIdle(cam);
    return CMUCAM4_RETURN_SUCCESS;
}

int CMUCam4_sendFrame(cmucam4_instance_t *cam, int horizontalResolution, int verticalResolution,
                       uint16_t * buffer,
                       size_t horizonalSize, size_t horizontalOffset,
                       size_t verticalSize, size_t verticalOffset)
{
    int errorValue; int serialBuffer0; int serialBuffer1;
    size_t indexX; size_t indexY; size_t resolutionX; size_t resolutionY;

    resolutionX = (CMUCAM4_FRAME_H_RES >> horizontalResolution);
    resolutionY = (CMUCAM4_FRAME_V_RES >> verticalResolution);

    if(snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "SF %d %d\r", horizontalResolution, verticalResolution)
    >= CMUCAM4_CMD_BUFFER_SIZE)
    {
        return CMUCAM4_COMMAND_OVERFLOW;
    }

    if((errorValue = _commandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    for(indexX = 0; indexX < resolutionX; indexX++)
    {
        _setReadTimeout(cam, CMUCAM4_NON_FS_TIMEOUT);

        _receiveData(cam);

        if((*(cam->_resBuffer)) == ':')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }

        switch(cam->_version)
        {
            case VERSION_100:
            case VERSION_101:

                if(strcmp(cam->_resBuffer, "DAT:") != 0)
                {
                    return CMUCAM4_UNEXPECTED_RESPONCE;
                }

                break;

            case VERSION_102:
            case VERSION_103:

                if(strcmp(cam->_resBuffer, "DAT: ") != 0)
                {
                    return CMUCAM4_UNEXPECTED_RESPONCE;
                }

                break;
        }

        for(indexY = 0; indexY < resolutionY; indexY++)
        {
            serialBuffer0 = (_readWithTimeout(cam) & 0xFF);
            serialBuffer1 = (_readWithTimeout(cam) & 0xFF);

            if((buffer != NULL) && (horizontalOffset <= indexX) &&
            ((indexX - horizontalOffset) < horizonalSize) &&
            (verticalOffset <= indexY) &&
            ((indexY - verticalOffset) < verticalSize))
            {
                buffer[((indexY - verticalOffset) * horizonalSize)
                + (indexX - horizontalOffset)]
                = ((uint16_t) (serialBuffer0 | (serialBuffer1 << 8)));
            }
        }

        if(_readWithTimeout(cam) != '\r')
        {
            return CMUCAM4_UNEXPECTED_RESPONCE;
        }
    }

    _waitForIdle(cam);
    return CMUCAM4_RETURN_SUCCESS;
}

/*******************************************************************************
* Private Functions
*******************************************************************************/

int _voidCommandWrapper(cmucam4_instance_t *cam, const char * command, unsigned long timeout)
{
    int errorValue;

    if((errorValue = _commandWrapper(cam, command, timeout)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _waitForIdle(cam);
    return CMUCAM4_RETURN_SUCCESS;
}


int _intCommandWrapper(cmucam4_instance_t *cam, const char * command, unsigned long timeout)
{
    int errorValue; int resultValue; int returnValue;

    if((errorValue = _commandWrapper(cam, command, timeout)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _receiveData(cam);
    resultValue = (sscanf(cam->_resBuffer, "%d ", &returnValue) == 1);

    _waitForIdle(cam);
    return resultValue ? returnValue : CMUCAM4_UNEXPECTED_RESPONCE;
}

int _commandWrapper(cmucam4_instance_t *cam, const char * command, unsigned long timeout)
{
    int errorValue;

    if((errorValue = CMUCam4_idleCamera(cam)))
    {
        return errorValue;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _setReadTimeout(cam, timeout);
    CMUcom4_write_str(&cam->_com, command);
    _waitForResponce(cam);

    return CMUCAM4_RETURN_SUCCESS;
}

int _responceWrapper(cmucam4_instance_t *cam, char responce)
{
    int errorValue;

    if(cam->_state == DEACTIVATED)
    {
        return CMUCAM4_NOT_ACTIVATED;
    }

    if((errorValue = setjmp(cam->_env)))
    {
        return errorValue;
    }

    _setReadTimeout(cam, CMUCAM4_NON_FS_TIMEOUT);

    for(;;)
    {
        _receiveData(cam);

        if((*cam->_resBuffer) == responce)
        {
            break;
        }

        if((*cam->_resBuffer) == ':')
        {
            return CMUCAM4_STREAM_END;
        }

        if(strcmp(cam->_resBuffer, "F ") == 0)
        {
            _readBinary(cam, NULL, 0, CMUCAM4_ID_T_LENGTH, 0);

            if(_readWithTimeout(cam) != '\r')
            {
                return CMUCAM4_UNEXPECTED_RESPONCE;
            }
        }
    }

    return CMUCAM4_RETURN_SUCCESS;
}

void _waitForIdle(cmucam4_instance_t *cam)
{
    for(;;)
    {
        _readText(cam);

        if(_startsWithString(cam, "MSG"))
        {
            continue; // Throw the message away.
        }

        _handleError(cam);

        if((*cam->_resBuffer) != ':')
        {
            longjmp(cam->_env, CMUCAM4_UNEXPECTED_RESPONCE);
        }

        break;
    }
}

void _waitForResponce(cmucam4_instance_t *cam)
{
    _readText(cam);

    if(strcmp(cam->_resBuffer, "NCK") == 0)
    {
        _readText(cam);

        if((*cam->_resBuffer) == ':')
        {
            longjmp(cam->_env, CMUCAM4_NCK_RESPONCE);
        }

        longjmp(cam->_env, CMUCAM4_UNEXPECTED_RESPONCE);
    }

    if(strcmp(cam->_resBuffer, "ACK") != 0)
    {
        longjmp(cam->_env, CMUCAM4_UNEXPECTED_RESPONCE);
    }
}

void _receiveData(cmucam4_instance_t *cam)
{
    for(;;)
    {
        _readText(cam);

        if(_startsWithString(cam, "MSG"))
        {
            continue; // Throw the message away.
        }

        _handleError(cam);

        break;
    }
}

void _handleError(cmucam4_instance_t *cam)
{
    int errorValue; int sum; size_t index; size_t length;

    if(_startsWithString(cam, "ERR"))
    {
        sum = 0; length = strlen(cam->_resBuffer);

        for(index = 0; index < length; index++)
        {
            sum += cam->_resBuffer[index];
        }

        switch(sum)
        {
            case CMUCAM4_CAMERA_TIMEOUT_ERROR_SUM:
                errorValue = CMUCAM4_CAMERA_TIMEOUT_ERROR; break;

            case CMUCAM4_CAMERA_CONNECTION_ERROR_SUM:
                errorValue = CMUCAM4_CAMERA_CONNECTION_ERROR; break;

            case CMUCAM4_DISK_IO_ERROR_SUM:
                errorValue = CMUCAM4_DISK_IO_ERROR; break;

            case CMUCAM4_FILE_SYSTEM_CORRUPTED_SUM:
                errorValue = CMUCAM4_FILE_SYSTEM_CORRUPTED; break;

            case CMUCAM4_FILE_SYSTEM_UNSUPPORTED_SUM:
                errorValue = CMUCAM4_FILE_SYSTEM_UNSUPPORTED; break;

            case CMUCAM4_CARD_NOT_DETECTED_SUM:
                errorValue = CMUCAM4_CARD_NOT_DETECTED; break;

            case CMUCAM4_DISK_MAY_BE_FULL_SUM:
                errorValue = CMUCAM4_DISK_MAY_BE_FULL; break;

            case CMUCAM4_DIRECTORY_FULL_SUM:
                errorValue = CMUCAM4_DIRECTORY_FULL; break;

            case CMUCAM4_EXPECTED_AN_ENTRY_SUM:
                errorValue = CMUCAM4_EXPECTED_AN_ENTRY; break;

            case CMUCAM4_EXPECTED_A_DIRECTORY_SUM:
                errorValue = CMUCAM4_EXPECTED_A_DIRECTORY; break;

            case CMUCAM4_ENTRY_NOT_ACCESSIBLE_SUM:
                errorValue = CMUCAM4_ENTRY_NOT_ACCESSIBLE; break;

            case CMUCAM4_ENTRY_NOT_MODIFIABLE_SUM:
                errorValue = CMUCAM4_ENTRY_NOT_MODIFIABLE; break;

            case CMUCAM4_ENTRY_NOT_FOUND_SUM:
                errorValue = CMUCAM4_ENTRY_NOT_FOUND; break;

            // For v1.02 firmware and above.
            case CMUCAM4_ENTRY_ALREADY_EXISTS_SUM:
                errorValue = CMUCAM4_ENTRY_ALREADY_EXISTS; break;

            // For v1.01 firmware and below.
            case (CMUCAM4_ENTRY_ALREADY_EXISTS_SUM - 's'):
                errorValue = CMUCAM4_ENTRY_ALREADY_EXISTS; break;

            case CMUCAM4_DIRECTORY_LINK_MISSING_SUM:
                errorValue = CMUCAM4_DIRECTORY_LINK_MISSING; break;

            case CMUCAM4_DIRECTORY_NOT_EMPTY_SUM:
                errorValue = CMUCAM4_DIRECTORY_NOT_EMPTY; break;

            case CMUCAM4_NOT_A_DIRECTORY_SUM:
                errorValue = CMUCAM4_NOT_A_DIRECTORY; break;

            case CMUCAM4_NOT_A_FILE_SUM:
                errorValue = CMUCAM4_NOT_A_FILE; break;

            default:
                errorValue = CMUCAM4_UNEXPECTED_RESPONCE; break;
        }

        _readText(cam);

        if((*cam->_resBuffer) == ':')
        {
            longjmp(cam->_env, errorValue);
        }

        longjmp(cam->_env, CMUCAM4_UNEXPECTED_RESPONCE);
    }
}

void _waitForString(cmucam4_instance_t *cam, const char * string)
{
    size_t index; size_t length = strlen(string);
    memset(cam->_resBuffer, '\0', CMUCAM4_RES_BUFFER_SIZE);

    do
    {
        for(index = 1; index < length; index++)
        {
            cam->_resBuffer[index - 1] = cam->_resBuffer[index];
        }

        cam->_resBuffer[length - 1] = _readWithTimeout(cam);
    }
    while(strcmp(cam->_resBuffer, string) != 0);
}


int _startsWithString(cmucam4_instance_t *cam, const char * string)
{
    return (strncmp(cam->_resBuffer, string, strlen(string)) == 0);
}

void _readBinary(cmucam4_instance_t *cam, uint8_t * buffer, size_t size,
                          unsigned long packetSize,
                          unsigned long packetOffset)
{
    int serialBuffer; unsigned long serialCounter;

    for(serialCounter = 0; serialCounter < packetSize; serialCounter++)
    {
        serialBuffer = _readWithTimeout(cam);

        if((buffer != NULL) && (packetOffset <= serialCounter) &&
        ((serialCounter - packetOffset) < ((unsigned long) size)))
        {
            buffer[serialCounter - packetOffset] = ((uint8_t) serialBuffer);
        }
    }
}

void _readText(cmucam4_instance_t *cam)
{
    int serialBuffer; size_t serialCounter = 0;
    memset(cam->_resBuffer, '\0', CMUCAM4_RES_BUFFER_SIZE);

    for(;;)
    {
        serialBuffer = _readWithTimeout(cam);

        if(serialBuffer == '\r')
        {
            break;
        }

        cam->_resBuffer[serialCounter++] = serialBuffer;

        if(serialCounter >= CMUCAM4_RES_BUFFER_SIZE)
        {
            longjmp(cam->_env, CMUCAM4_RESPONCE_OVERFLOW);
        }

        switch(serialCounter)
        {
            case sizeof(char):

                if((*cam->_resBuffer) == ':')
                {
                    return; // Found the idle character.
                }

                break;

            case (sizeof("F ") - 1):

                if(strcmp(cam->_resBuffer, "F ") == 0)
                {
                    return; // Found type F packet.
                }

                break;

            case (sizeof("DAT:") - 1):

                if(cam->_state == ACTIVATED)
                {
                    switch(cam->_version)
                    {
                        case VERSION_100:
                        case VERSION_101:

                            if(strcmp(cam->_resBuffer, "DAT:") == 0)
                            {
                                return; // Found a old style DAT packet.
                            }

                            break;

                        case VERSION_102:
                        case VERSION_103:

                            break;
                    }
                }

                break;

            case (sizeof("DAT: ") - 1):

                if(cam->_state == ACTIVATED)
                {
                    switch(cam->_version)
                    {
                        case VERSION_100:
                        case VERSION_101:

                            break;

                        case VERSION_102:
                        case VERSION_103:

                            if(strcmp(cam->_resBuffer, "DAT: ") == 0)
                            {
                                return; // Found a new style DAT packet.
                            }

                            break;
                    }
                }

                break;

            default: break;
        }
    }
}

void _setReadTimeout(cmucam4_instance_t *cam, unsigned long timeout)
{
    cam->_timeout = timeout;
    cam->_milliseconds = CMUcom4_milliseconds(&cam->_com);
}

int _readWithTimeout(cmucam4_instance_t *cam) //pass in parameter saying how much data to expect, set rx_handler with trigger level
{
	uint32_t rx_size = 0;
    do
    {
        //if((CMUcom4_milliseconds(&cam->_com) - cam->_milliseconds) >= cam->_timeout) {
    	//	longjmp(cam->_env, CMUCAM4_SERIAL_TIMEOUT);
    	//}
    	rx_size = MSS_UART_get_rx (
        		cam->_com.uart, // UART Instance
        		cam->_com.buffer, // Pointer to receive data buffer
        		sizeof(cam->_com.buffer) // Size of the data receive buffer
            );
    }
    while( rx_size == 0);

    return *cam->_com.buffer;
}
