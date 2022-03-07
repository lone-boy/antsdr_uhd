// Global include file for MicroZedAXIStreamEthernetUDP project

// Debug defines
#define READ_BACK_INDEX_ARRAY

// Define port to listen on
#define FF_UDP_PORT 49200
#define DATA_DDP_PORT 49202

// TIMEOUT FOR DMA AND GMM WAIT
#define RESET_TIMEOUT_COUNTER	10000

// DEFINES
#define WAVE_SIZE_BYTES    512  // Number of samples in waveform 
#define INDARR_SIZE_BYTES  1024 // Number of bytes required to hold 512 fixed point floats

//HARDWARE DEFINES
#define NUMCHANNELS 		2	// Number of parallel operations done on input stream (1 OR 2)
#define BW   				32	// Total number of bits in fixed point data type
#define IW    				24	// Number of bits left of decimal point in fixed point data type
#define BITDIV			 256.0 	// Divisor to shift fixed point to int and back to float


#define MICROPHASE_E310_F2_COMPAT_NUM 2

typedef enum{
	MICROPHASE_CTRL_ID_HUH_WHAT = ' ',

	MICROPHASE_CTRL_ID_WAZZUP_BR0 = 'm',
	MICROPHASE_CTRL_ID_WAZZUP_DUDE = 'M',
} microphase_e310_ctrl_id_e;

typedef struct{
    uint32_t proto_ver;
    uint32_t id;
    uint32_t seq;
    union {
        uint32_t ip_addr;
        struct{
            uint32_t dev;
            uint32_t data;
            uint8_t miso_dege;
            uint8_t mosi_edge;
            uint8_t num_bits;
            uint8_t readback;
        } spi_args;
        struct {
            uint8_t addr;
            uint8_t bytes;
            uint8_t data[20];
        } i2c_args;
        struct {
            uint32_t addr;
            uint32_t data;
            uint8_t action;
        } reg_args;
        struct {
            uint32_t len;
        } echo_args;
    } data;
} microphase_e310_ctrl_data_t;

typedef struct{
	uint32_t head;
	uint32_t sid;
	uint64_t timer;
	uint64_t data;

}microphase_e310_ctrl_send_uhd_t;



