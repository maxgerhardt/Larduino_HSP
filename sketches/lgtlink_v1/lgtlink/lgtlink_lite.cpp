//===================================================
// lgtlink_lite project (arduino based)
// -------------------------------------------------
// Description:
// a open source isp for lgtmcu
// lite version: w/o vpp surport
// -------------------------------------------------
// Revision 
//  1.0  2018/10/29 Franz
//    - initial version
//==================================================== 
// 10pin (2x5) connector definition:
//    swc/txd/tck   -1    2- gnd
//    rstn/trstn    -3    4- tms/ts1
//    swd/rxd/tdi   -5    6- tdo
//            vdd   -7    8- vpp(n.c)
//            gnd   -9   10- gnd
// ----------------------------------------------------
// 6pin (1x6) connector definition: (w/o jtag)
//          1- swc/txd
//          2- rstn/trstn
//          3- swd/rxd
//          4- vdd
//          5- gnd
//          6- vpp
// ----------------------------------------------------
// I/O assignment:
//      swc/txd/tck - PB5 : out
//      swd/rxd/tdi - PB4 : inout
//        tms/ts1   - PB3 : out
//            tdo   - PB1 : out
//       rstn/trstn - PC0 : out
// -----------------------------------------------------       
// I/O timing frame:
//  [ 2bits opc ] + [ PB5 PB4 PB3 PB2 PB1 PB0 ]

#define   LGTLINK_MAX_CMD	9

#define   LGTLINK_OK		0
#define   LGTLINK_ERR_CMD	1
#define   LGTLINK_ERR_FSM	2
#define   LGTLINK_ERR_CKSUM	3

#define IO_RESET_ON() PORTC &= ~(1 << 0)
#define IO_RESET_OFF() PORTC |= (1 << 0);
#define IO_WRITE(value) do {\
	PORTB = (PORTB & io_odir_bmask) | value;\
} while(0)

#define IO_READ() (PINB & io_idata_bmask)

static uint8_t io_odir_bmask;
static uint8_t io_idir_bmask;
static uint8_t io_band_rate;
static uint8_t io_idata_bmask;

// data fifo
union _dfifo {
	uint32_t wdfifo[32];
	uint16_t hdfifo[64];
	uint8_t bdfifo[128];
} d_fifo;

// timing macro fifo
uint8_t g_mfifo[16][8];

// commands fifo
uint8_t g_cfifo[16];

// char from upstream
int  g_wc;
char g_uc;
uint8_t g_fsm;
uint8_t g_busy;
uint8_t g_errno;


void lgtlink_init()
{
	io_odir_bmask = 0xff;
	io_idir_bmask = 0xff;
	io_band_rate = 0x20;  // for 500K swc/tck
	io_idata_bmask = 0x10; // PB4 for data input

	g_fsm = 0; // main fsm
	g_busy = 0;
	g_errno = 0;  
}

void setup() 
{
	// put your setup code here, to run once:
	Serial.begin(115200);

	lgtlink_init();
}

void loop() 
{
	// put your main code here, to run repeatedly:

	while(1) {
		// waiting start
		g_wc = Serial.read();
		if(g_wc < 0) continue;
		g_uc = (char) g_wc;

		if(g_uc != '#') continue;

		// waiting cmd
		g_wc = Serial.read();
		if(g_wc < 0) continue;

		g_uc = (char) g_wc;

		switch(g_uc)
		case 0x00:	
			lgtlink_init();
			break;
		case 0x03: 
			lgtlink_io_reset();
			break;
		case 0x0f:
			lgtlink_cmd_start();
			break;
		case 0xe1:
			lgtlink_set_config();
			break;
		case 0xd2:
			lgtlink_dfifo_write();
			break;
		case 0xc3:
			lgtlink_dfifo_read();
			break;
		case 0xb4:
			lgtlink_mfifo_write();
			break;
		case 0xa5
			lgtlink_cfifo_write();
			break;
		default: 
			g_errno = LGTLINK_ERR_FSM;			
		break;

		// exception
		if(g_errno != LGTLINK_OK) {
			Serial.Write("?");
			Serial.Write(g_uc, HEX);
			g_errno = LGTLINK_OK;
		}
	}
}
