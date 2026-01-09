#include <stdio.h>
#include <stdint.h>
#include <string.h>

// microsoft proprietary
#include <direct.h>

#define APP_NAME "mobileExtract.exe"

// globals: the sign you're dealing with quality code!
uint8_t* spr; 
uint8_t* pag;
uint8_t is_big_block;

char* mobilename = "MobileA"; // last char will be overwritten as execution continues... great job

char base_path[1024];

void FUN_004026b0() {
    // ???
}

void FUN_00402230() {
    // ???
}

#define M_Get16Le(bufptr) (bufptr[0] | (bufptr[1] << 8) )

void try_make_directory(char* path) {
    int *piVar1;
    int error_code;
    undefined4 local_420;
    char local_41c [16];
    char path_candidate [1032];
    int attempts = 0;

    strcpy(path_candidate,path);

    error_code = mkdir(path_candidate);

    // amazingly terrible code that will try to create e.g., "nanddump_out1"
    // if "nanddump_out" already exists
    while(error_code != 0) {
        if (errno == EEXIST) {
            strcpy(path_candidate,path);
            sprintf(local_41c,"%d",attempts);
            strcat(path_candidate,local_41c);
            error_code = mkdir(path_candidate);
            attempts++;
        } else {
            printf("**** making directory %s returned %d (err: %d)\n",path_candidate, error_code, local_420);

            // this infinite loop hazard is present in the original code
        }
    }

    strcpy(path, path_candidate);
}

void FUN_0040151a(char* file_path) {
    char local_9c [128];
    char* local_1c;
    char* local_18;
    int local_14;
    undefined *local_10;

    memset(local_9c, 0, 128);
    strcpy(base_path, file_path);

    // very shitty code that looks for a base directory path and the file extension
    // e.g., "C:\nanddump.bin"
    local_18 = strrchr(base_path, '\\'); // windows-specific behavior; will NOT work on unix.
    local_1c = strrchr(base_path, '.');
    local_14 = (local_1c == 0) ? strlen(local_1c) : local_1c - local_18;

    // from "C:\nanddump.bin" we get "nanddump"
    strncpy(local_9c, local_18, local_14);
    strcat(local_9c, "_out");

    // some code that ghidra's decompiler chokes on
    //     004015d8 83 7d ec 00     CMP        dword ptr [EBP + local_18],0x0
    // 004015dc 74 18           JZ         LAB_004015f6
    // 004015de 8b 45 ec        MOV        EAX,dword ptr [EBP + local_18]
    // 004015e1 2d 10 50        SUB        EAX,DAT_00405010                                           = ??
    //          40 00
    // 004015e6 89 45 f4        MOV        dword ptr [EBP + local_10],EAX
    // 004015e9 8b 45 f4        MOV        EAX,dword ptr [EBP + local_10]
    // 004015ec 05 10 50        ADD        EAX,DAT_00405010                                           = ??
    //          40 00
    // 004015f1 c6 00 00        MOV        byte ptr [EAX],0x0
    // 004015f4 eb 07           JMP        LAB_004015fd
    //                      LAB_004015f6                                    XREF[1]:     004015dc(j)  
    // 004015f6 c6 05 10        MOV        byte ptr [DAT_00405010],0x0                                = ??
    //          50 40 00 00
    //                      LAB_004015fd                                    XREF[1]:     004015f4(j)  
    // 004015fd 8d 85 68        LEA        EAX=>local_9c,[EBP + 0xffffff68]
    //          ff ff ff

    strcat(base_path, local_9c);
    try_make_directory(base_path);
    strcat(base_path, "\\");
}

FILE* auto_open(char* name, char* mode) {
    char buf[1032];
    strcpy(buf, base_path);
    strcat(buf, name);
    return fopen(buf, mode);
}

void find_and_dump_mobiles(int max_pages) {
    char fn[40];

    // MobileX.dat does not actually live in the filesystem,
    // it lives in a fixed range of blocks
    for (int i = 0x31; i < 0x3F; i++) {
        // look for the block location, and if it isn't found, skip it.
        int mobile_location = FUN_00401a5d(max_pages, i);
        if (mobile_location == 0) continue;

        uint8_t* spr_ptr = spr + i * 0x10;

        // 00401cbc 8b 45 b4        MOV        EAX,dword ptr [EBP + local_50]
        // 00401cbf 0f b6 40 08     MOVZX      EAX,byte ptr [EAX + 0x8]
        // 00401cc3 89 c2           MOV        EDX,EAX
        // 00401cc5 c1 e2 08        SHL        EDX,0x8
        // 00401cc8 8b 45 b4        MOV        EAX,dword ptr [EBP + local_50]
        // 00401ccb 0f b6 40 07     MOVZX      EAX,byte ptr [EAX + 0x7]
        // 00401ccf 8d 04 02        LEA        EAX,[EDX + EAX*0x1]
        // 00401cd2 89 45 c4        MOV        dword ptr [EBP + local_40],EAX
        int mobile_size = M_Get16Le(spr_ptr + 7);

        // yes, this is really what the code is doing, it's a horrible hack
        mobilename[6] = i + 0x11;
        sprintf(fn, "%s.dat", mobilename);

        printf("Dumping: %s found at page 0x%x, size %d (0x%x) bytes\n", fn, mobile_location, mobile_size, mobile_size);

        FILE* fout = auto_open(fn, "wb");
        fwrite(pag + mobile_location * 0x200, mobile_size, 1, fout);
        fclose(fout);
    }
}

int FUN_00401a5d(int max_pages, int mobile_id) {
    uint uVar1;
    ushort uVar2;
    
    int iVar3; // =local_28

    undefined4 local_1c;
    undefined4 local_10;
    undefined4 local_c;

    local_10 = 0;
    local_1c = 0;
    local_c = 0;
    while (local_c < max_pages) {
        int spr_offset = local_c * 0x10;  // =local_14
        int pag_offset = local_c * 0x200; // =local_18

        iVar3 = spr + spr_offset;

        // this looks extremely weird, but the mobile pages do NOT use the standard ECC codes.
        if ((*(byte *)(iVar3 + 0xc) & 0x3f) == mobile_id) {
            if (!is_big_block) {
            
                uVar1 = (uint)*(ushort *)(iVar3 + 2) + (uint)*(byte *)(iVar3 + 4) * 0x10000;
            } else {
                uVar1 = (uint)CONCAT11(*(undefined *)(iVar3 + 4),*(undefined *)(iVar3 + 5)) +
                    (uint)*(byte *)(iVar3 + 3) * 0x10000;
            }

            // code paths are the same between big block and normal NANDs, but the code is duplicated
            // in the compiled source...

            if (local_1c <= uVar1) {
                local_10 = local_c;
                local_1c = uVar1;
            }
            
            uVar2 = *(ushort *)(iVar3 + 7);
            if (uVar2 >> 9 < 4) {
                local_c += 4;
            } else {
                if ((uVar2 & 0x1ff) != 0) {
                    local_c++;
                }
                local_c = local_c + (uVar2 >> 9);
            }
        } else {
            local_c += 4;
        }
    }

    return local_10;
}

void usage() {
  printf("%s\n", APP_NAME);
  printf("---------------------------------------\n");
  printf("Invalid, drag and drop file to extract.\n");
  printf("Usage:\t%s\n",APP_NAME);
  printf("---------------------------------------\n");
  return;
}

inline void prompt_exit() {
    printf("\npress <enter> to quit...\n");
    getchar(); // actually fgetc(_iob_exref);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
        goto _exit;
    }

    FILE* f = fopen(argv[1], "rb");
    if (f == NULL) {
        printf("\n\n** Error opening %s.\n",argv[1]);
        goto _exit;
    }

    fseek(f, 0, SEEK_END);
    size_t nand_size = ftell(f);
    rewind(f);

    if ((nand_size % 0x4200) != 0) {
        fclose(f);
        printf("\n\n** Input file is not the correct size.\n");
        goto _exit;
    }

    int nand_blocks = nand_size / 0x4200;

    // cap nand size at 64mbytes
    if (nand_blocks >= 0x1000) {
        nand_blocks = 0x1000;
        nand_size = 0x4200000;
    }

    // code assumes 32 pages per block
    unsigned long spr_size = nand_blocks * 0x200;
    unsigned long pag_size = nand_blocks * 0x4000;

    printf("%s\n-----------------------------------\n",APP_NAME);

    FUN_0040151a(argv[1]);

    printf("Input File  :\n %s\n",argv[1]);
    printf("Total Blocks:\n %d\n",nand_blocks);

    spr = malloc(spr_size);
    if (spr == NULL) {
        fclose(f);
        printf("\n\n** Error allocating spr.\n");
        goto _exit;
    }

    pag = malloc(pag_size);
    if (pag == NULL) {
        fclose(f);
        printf("\n\n** Error allocating pag.\n");
        goto _exit;
    }

    printf("Reading data now....please wait\n");
    for (int i = 0; i < nand_blocks * 32; i++) {
        // pages and ECC data are read seperately
        fread(pag + i * 0x200, 1, 0x200, f);
        fread(spr + i * 0x10,  1, 0x10,  f);
    }
    fclose(f);

    is_big_block = 0;
    if (spr[0] == 0xFF && spr[1] == 0x00) {
        is_big_block = 1;
        printf("big block spare detected!\n");
    }
    
    find_and_dump_mobiles(nand_blocks * 32);

    free(spr);
    free(pag);
                                   
    printf("\n%s Finished.\n", APP_NAME);

_exit:
    prompt_exit();
    return 0;
}