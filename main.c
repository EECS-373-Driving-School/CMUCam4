#include "drivers/mss_uart/mss_uart.h"
#include "cmucam4.h"

int main()
{
	cmucam4_instance_t *cam = &cmucam4;
	char str[36];

	printf("start\r\n");

	CMUCam4_init(cam, &g_mss_uart1);

	CMUCam4_cmd(cam, "L1", "2");
	CMUCam4_cmd(cam, "GV", "");

	CMUCam4_copy_data(cam, &str, sizeof(str));

	printf(str);

	while( 1 )
	{

	}
}