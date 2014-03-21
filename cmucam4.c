#include "CMUcam4.h"

cmucam4_instance_t cmucam4;

void CMUcam4_init ( cmucam4_instance_t *cmucam4)
{
	cmucom4_instance_t _copy;
	CMUcom4_init(_copy, g_mss_uart1);
	cmucam4->state = DEACTIVATED;
    cmucam4->_com = _copy;
}

void CMUcam4_init ( cmucam4_instance_t *cmucam4, mss_uart_instance_t* uart )
{
	cmucom4_instance_t _copy;
	CMUcom4_init(_copy, uart);
	cmucam4->state = DEACTIVATED;
    cmucam4->_com = _copy;
}

/*******************************************************************************
* Helper Functions
*******************************************************************************/

int getPixel( CMUcam4_image_data_t * pointer,
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

int setPixel(CMUcam4_image_data_t * pointer,
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

int andPixels(CMUcam4_image_data_t * destination,
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

int orPixels(CMUcam4_image_data_t * destination,
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

int xorPixels(CMUcam4_image_data_t * destination,
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

int notPixels(CMUcam4_image_data_t * destination)
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

int isReadOnly(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->readOnly == 'R');
}

int isHidden(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->hidden == 'H');
}

int isSystem(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->system == 'S');
}


int isVolumeID(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->volumeID == 'V');
}

int isDirectory(CMUcam4_directory_entry_t * pointer)
{
    CMUcam4_entry_attributes_t * attributes;

    if(pointer == NULL)
    {
        return CMUCAM4_RETURN_FAILURE;
    }

    attributes = ((CMUcam4_entry_attributes_t *) pointer->attributes);
    return (attributes->directory == 'D');
}

int isArchive(CMUcam4_directory_entry_t * pointer)
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

int begin(cmucam4_instance_t *cam)
{
    int errorValue; int retVal0; int retVal1; static int resetTries;

    resetTries = CMUCAM4_RESET_TRIES;
    errorValue = setjmp(cam->_env);

    if(resetTries-- <= 0)
    {
        cam->_com.end();
        return errorValue;
    }

    // Try to reset at fast, medium, and slow baud rates.

    cam->_state = DEACTIVATED;
    _setReadTimeout(cam, CMUCAM4_RESET_TIMEOUT);

    if(errorValue)
    {
        cam->_com.end();
    }

    cam->_com.begin(resetTries ? CMUCOM4_FAST_BAUD_RATE : CMUCOM4_MEDIUM_BAUD_RATE);
    cam->_com.write((uint8_t) '\0');
    cam->_com.write((uint8_t) '\0');
    cam->_com.write((uint8_t) '\0');
    cam->_com.write("\rRS\r");

    cam->_com.end();
    cam->_com.begin(CMUCOM4_MEDIUM_BAUD_RATE);
    cam->_com.write((uint8_t) '\0');
    cam->_com.write((uint8_t) '\0');
    cam->_com.write((uint8_t) '\0');
    cam->_com.write("\rRS\r");

    cam->_com.end();
    cam->_com.begin(CMUCOM4_SLOW_BAUD_RATE);
    cam->_com.write((uint8_t) '\0');
    cam->_com.write((uint8_t) '\0');
    cam->_com.write((uint8_t) '\0');
    cam->_com.write("\rRS\r");

    // Get the firmware version.

    _waitForString(cam, "\rCMUcam4 v");
    _readText(cam);

    if(sscanf(cam->_resBuffer, "%1d.%2d ", &retVal0, &retVal1) != 2)
    {
        longjmp(cam->_env, CMUCAM4_UNEXPECTED_RESPONCE);
    }

    cam->_version = ((_CMUcam4_version) ((retVal0 * 100) + retVal1));

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
    cam->_com.write("BM ");

    switch(cam->_version)
    {
        case VERSION_100:
        case VERSION_101: _com.write(CMUCOM4_MEDIUM_BR_STRING); break;
        case VERSION_102:
        case VERSION_103: _com.write(resetTries ?
                                     CMUCOM4_FAST_BR_STRING :
                                     CMUCOM4_MEDIUM_BR_STRING); break;
    }

    cam0>_com.write((uint8_t) '\r');
    _waitForResponce(cam);
    cam->_com.end();

    switch(cam->_version)
    {
        case VERSION_100:
        case VERSION_101: _com.begin(CMUCOM4_MEDIUM_BAUD_RATE); break;
        case VERSION_102:
        case VERSION_103: _com.begin(resetTries ?
                                     CMUCOM4_FAST_BAUD_RATE :
                                     CMUCOM4_MEDIUM_BAUD_RATE); break;
    }

    cam->_com.write((uint8_t) '\r');
    _waitForResponce(cam);
    _waitForIdle(cam);

    // Adjust the stop bits.

    _setReadTimeout(cam, CMUCAM4_NON_FS_TIMEOUT);
    cam->_com.write("DM ");

    switch(cam->_version)
    {
        case VERSION_100:
        case VERSION_101: _com.write(CMUCOM4_MEDIUM_SB_STRING); break;
        case VERSION_102:
        case VERSION_103: _com.write(resetTries ?
                                     CMUCOM4_FAST_SB_STRING :
                                     CMUCOM4_MEDIUM_SB_STRING); break;
    }

    cam->_com.write((uint8_t) '\r');
    _waitForResponce(cam);
    _waitForIdle(cam);

    cam->_state = ACTIVATED;
    return CMUCAM4_RETURN_SUCCESS;
}

int end(cmucam4_instance_t *cam)
{
    if(cam->_state == DEACTIVATED)
    {
        return CMUCAM4_NOT_ACTIVATED;
    }

    cam->_state = DEACTIVATED;
    cam->_com.end();

    return CMUCAM4_RETURN_SUCCESS;
}

/*******************************************************************************
* System Level Commands
*******************************************************************************/

int getVersion(cmucam4_instance_t *cam)
{
    return (cam->_state == ACTIVATED) ? _version : CMUCAM4_NOT_ACTIVATED;
}

int resetSystem(cmucam4_instance_t *cam)
{
    return (cam->_state == ACTIVATED) ? begin() : CMUCAM4_NOT_ACTIVATED;
}

int sleepDeeply(cmucam4_instance_t *cam)
{
    return _commandWrapper(cam, "SD\r", CMUCAM4_NON_FS_TIMEOUT);
}

int sleepLightly(cmucam4_instance_t *cam)
{
    return _commandWrapper(cam, "SL\r", CMUCAM4_NON_FS_TIMEOUT);
}

/*******************************************************************************
* Camera Module Commands
*******************************************************************************/

int cameraBrightness(cmucam4_instance_t *cam, int brightness)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CB %d\r", brightness) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int cameraContrast(cmucam4_instance_t *cam, int contrast)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CC %d\r", contrast) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int cameraRegisterRead(cmucam4_instance_t *cam, int reg)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CR %d\r", reg) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _intCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int cameraRegisterWrite(cmucam4_instance_t *cam, int reg, int value, int mask)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "CW %d %d %d\r", reg, value, mask) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Camera Sensor Auto Control Commands
*******************************************************************************/

int autoGainControl(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "AG %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int autoWhiteBalance(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "AW %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Camera Format Commands
*******************************************************************************/

int horizontalMirror(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "HM %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int verticalFlip(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "VF %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}


/*******************************************************************************
* Camera Effect Commands
*******************************************************************************/

int blackAndWhiteMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "BW %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int negativeMode(cmucam4_instance_t *cam, int active)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "NG %d\r", active) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

/*******************************************************************************
* Auxiliary I/O Commands
*******************************************************************************/

int getButtonState(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GB\r", CMUCAM4_NON_FS_TIMEOUT);
}

long getButtonDuration(cmucam4_instance_t *cam)
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

int getButtonPressed(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GP\r", CMUCAM4_NON_FS_TIMEOUT);
}

int getButtonReleased(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GR\r", CMUCAM4_NON_FS_TIMEOUT);
}

int panInput(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "PI\r", CMUCAM4_NON_FS_TIMEOUT);
}

int panOutput(cmucam4_instance_t *cam, int direction, int output)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "PO %d %d\r", direction, output) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int tiltInput(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "TI\r", CMUCAM4_NON_FS_TIMEOUT);
}

int tiltOutput(cmucam4_instance_t *cam, int direction, int output)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "TO %d %d\r", direction, output) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int getInputs(cmucam4_instance_t *cam)
{
    return _intCommandWrapper(cam, "GI\r", CMUCAM4_NON_FS_TIMEOUT);
}

int setOutputs(cmucam4_instance_t *cam, int directions, int outputs)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "SO %d %d\r", directions, outputs) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}

int LEDOff(cmucam4_instance_t *cam)
{
    return _voidCommandWrapper(cam, "L0\r", CMUCAM4_NON_FS_TIMEOUT);
}

int LEDOn(cmucam4_instance_t *cam, long frequency)
{
    return (snprintf(cam->_cmdBuffer, CMUCAM4_CMD_BUFFER_SIZE,
    "L1 %ld\r", frequency) < CMUCAM4_CMD_BUFFER_SIZE)
    ? _voidCommandWrapper(cam, cam->_cmdBuffer, CMUCAM4_NON_FS_TIMEOUT)
    : CMUCAM4_COMMAND_OVERFLOW;
}









