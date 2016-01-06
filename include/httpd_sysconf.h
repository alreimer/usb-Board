
//========USB PORT struct variable definition=============
typedef struct{
    char device_name[17][20];
    char folder_name[17][20];
    char system_type[17][20];
    char series[17][7];
    }USB_PORT;
USB_PORT usb_port[4];

typedef struct{
    char device_name[20];
    char folder_name[20];
    char system_type[10];
    char folder_name_tmp[20];
    char folder_name_check[20];
    char device_name_adjust[20];
    char folder_name_adjust[20];
}MOUNT_VALUE;

//========USB struct variable definition=============
typedef struct{
    int partition;
    char device[9][20];
    char type[9][20];
    char size[9][20];
    }USB_DISK;

//allen adds to solve usb hd format
typedef struct{
    char hddevice[5];
    char umount[2];
}USB;
USB usb[2];  //allen adds to solve usb hd format

typedef struct{
    char server[2];
    //char server2[2];
    char scrpage[5];
}USB_PRINTER;
USB_PRINTER usb_printer;
//===================================================
char command_1[256];
//========Wget struct variable definition============
typedef struct{
    char address[160]; //only 128 is allowed but we allocates 30 extra to accomodate ten "/", which are coverted into %2F(3bytes)
    char account[21];
    char password[21];
    char accesspath[17];
    char message[6];
    char pid[10];
    char status[2];
    char del[2];
}WGET_HISTORY;
WGET_HISTORY wget_history[5];

typedef struct{
    char address[160];
    char account[21];
    char password[21];
    char accesspath[17];
    char message[6];
    char pid[10];
    char status[2];
    char del[2];
    char percent[4];
}WGET_DOWNLOAD;
WGET_DOWNLOAD wget_download[5];

typedef struct{
    char address[160];
    char account[21];
    char password[21];
    char accesspath[17];
    char message[6];
    char pid[10];
    char status[2];
    char del[2];
}WGET_VALUE;
WGET_VALUE wget_value[13];
//===================================================

typedef struct{
    char mac1[2];
    char mac2[2];
    char mac3[2];
    char mac4[2];
    char mac5[2];
    char mac6[2];
}MAC_VALUE;

typedef struct{
	char username[17];
	char user_kind[2];
	char user_ftp[2];
}USER_VALUE;

typedef struct{
    char foldername[17];
    char descrip[51];
    char kind;
}FOLDER_VALUE;

typedef struct{
	char username[17];
	char descrip[51];
}USER_DES_VALUE;

typedef struct{
    char name[17];
    char name_tmp[17]; //allen adds to declare temperature variable
    char description[51];
    char user[50][17];
    char kind[2];
    char kind_tmp[2]; //allen adds to declare temperature variable
    char kind_temp[2]; //allen adds to declare temperature variable
}GROUP_VALUE;
