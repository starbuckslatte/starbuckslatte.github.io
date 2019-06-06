
#include "stdio.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c  "
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

const int test_area_size = 64;

int testArea[test_area_size] =
{
	0xAA, 0xAA, 0xAA, 0xFA, 0xAF, 0xFF, 0xFF, 0xAA, 0xFA, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

int loop = 0;
bool check_second_byte = 0;
unsigned int word_current[32] = {0};
unsigned int first_byte = 0xFF;
unsigned int second_byte = 0xFF;

#define change_fw_and_reboot 1
#define nothing_change 0

void show_data()
{
    int i;
    int cnt = 0;
    for (i = 0; i < test_area_size; i++) {
        printf("%02X:", testArea[i]);
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(testArea[i]));
        cnt++;
        if (cnt == 8) {
            printf("\n");
            cnt = 0;
        }
    }
    printf("\n");
}

int get_dword(int idx)
{
    idx = idx * 4;
    return (testArea[idx] | ((testArea[idx + 1] << 8) & 0xFF00) | ((testArea[idx + 2] << 16) & 0xFF0000) | ((testArea[idx + 3] << 24) & 0xFF000000));
}

void set_dword(int idx, int value)
{
    idx = idx * 4;
    testArea[idx] = value & 0xFF;
    testArea[idx + 1] = (value >> 8 & 0xFF);
    testArea[idx + 2] = (value >> 16 & 0xFF);
    testArea[idx + 3] = (value >> 24 & 0xFF);
}

void save_in_flash(unsigned int byte_to_save, int loop)
{
    word_current[loop] = byte_to_save;
}

int byte_check(unsigned int byte_to_check, int loop)
{
    int ret = 0;

    if (byte_to_check == 0xFFFF) {
        set_dword(loop, 0x7FFF);
        ret = nothing_change;
    } else if (byte_to_check == 0x7FFF) {
        save_in_flash(0x5FFF, loop);
        ret = nothing_change;
    } else if (byte_to_check == 0x5FFF) {
        save_in_flash(0x57FF, loop);
        ret = nothing_change;
    } else if (byte_to_check == 0x57FF) {
        save_in_flash(0x03FF, loop);
        ret = change_fw_and_reboot;
    } else if (byte_to_check == 0x03FF) {
        save_in_flash(0x01FF, loop);
        ret = nothing_change;
    } else if (byte_to_check == 0x3FFF) {
        save_in_flash(0x1FFF, loop);
        ret = nothing_change;
    } else if (byte_to_check == 0x1FFF) {
        save_in_flash(0x17FF, loop);
        ret = nothing_change;
    } else if (byte_to_check == 0x17FF) {
        save_in_flash(0x0BFF, loop);
        ret = nothing_change;
    } else if (byte_to_check == 0x07FF) {
        save_in_flash(0x05FF, loop);
        ret = nothing_change;
    } else {
        //Flash Crash;
        int i = 0;
        for (i = 0; i < 32; i++) {
            word_current[i] = 0;
        }
    }
    return ret;
}

// **************************************************
//  return value
//  0 : no thing change
//  1 : change
// **************************************************
void bootloader_error_counter_check()
{
    // **************************************************
    //  1. Check and clear error counter flash area data
    //  2. Detect error counter number of bit is zero
    //  3. set error counter boorloader bit 1->0
    // **************************************************

    int ret = 0;

    // **************************************************
    //  Step 0: Initial Data;
    // **************************************************
    for (loop = 0; loop < 32; loop++) {
        word_current[loop] = 0xFFFF;
        printf(">>>>> word_current[%d]: 0X%x\r\n", loop, word_current[loop]);
    }

    // **************************************************
    //  Step 1: Read 32 Bytes data;
    // **************************************************
    for (loop = 0; loop < 32; loop++) {
        ret = byte_check(word_current[loop], loop);
        if (ret == change_fw_and_reboot) {
            printf(">>>>> change_fw_and_reboot\r\n");
            break;
        } else {
            printf(">>>>> word_current[%d]: 0X%x\r\n", loop, word_current[loop]);
            break;
        }
    }
    // **************************************************
    //  Step 2 : Extract First byte;
    // **************************************************
}

void fw_error_counter_func()
{
    int i, j, temp, checkClear, checkisEnd, need_2_set = 0, is_end = 0;
    int dword_num = test_area_size / 4;
    for (i = 0; i < dword_num; i++)////get doubleword, size/4
    //for(i=0;i<1;i++)
    {
        need_2_set = 0;
        temp = get_dword(i);
        if (temp == 0xFFFFFFFF)
            continue;
        printf("get temp: 0x%X \n", temp);

        for (j = 0; j < 16; j++) {
            //0x2 0x8,0x20, 0x80
            checkClear = (0xFFFFFFFE << (j * 2)) | (0xFFFFFFFE >> (32 - (j * 2)));
            printf("\n(%d:%d) (1) checkClear: 0x%X, ", i , j, checkClear);

            checkClear = temp & checkClear;
            printf("\n(%d:%d) (2) checkClear: 0x%X, ", i , j,checkClear);

            checkisEnd = (0x3 << (j * 2));
            printf("\n(%d:%d) (3) checkisEnd: 0x%X, ", i , j,checkisEnd);

            checkisEnd = temp | checkisEnd;
            printf("\n(%d:%d) (4) checkisEnd: 0x%X, ", i , j,checkisEnd);

            if (temp == checkClear) {
                printf("clear\n");
                //clear bits
                checkClear = (0xFFFFFFFC << (j * 2)) | (0xFFFFFFFC >> (32 - (j * 2)));
                temp = temp & checkClear;
                need_2_set = 1;
            }
            if (temp == checkisEnd) {
                is_end = 1;
                break;
            }
        }
        if (need_2_set) {
            printf("temp:0x%X\n", temp);
            set_dword(i, temp);
        }
        if (is_end) {
            break;
        }
    }
}

int main()
{
    printf("go\n");
    show_data();
    //bootloader_error_counter_check();
    fw_error_counter_func();
    show_data();
    printf("end\n");
    getchar();
    return 0;
}
