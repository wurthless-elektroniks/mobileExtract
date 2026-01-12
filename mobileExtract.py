'''
mobileExtract converted to Python to make it cross-platform and easier to maintain
'''

import struct
import os
import sys

APP_NAME = "mobileExtract"

def usage():
    print(f"{APP_NAME}")
    print("---------------------------------------")
    print("Invalid, drag and drop file to extract.")
    print(f"Usage:\t{APP_NAME}")
    print("---------------------------------------")

def find_mobile_block(max_pages: int,
                      mobile_id: int,
                      is_big_block: bool,
                      spr: bytes):
    i = 0
    local_10 = 0
    local_1c = 0

    while i < max_pages:
        spr_offset = i * 0x10

        # this looks odd if you assumed that the ECC field is 32 bits. but it's not.
        # the ECC field is 26 bits, leaving 6 bits free for some sort of metadata,
        # and in this case the settings blobs use it to identify themselves.
        if spr[spr_offset + 0x0C] & 0x3F == mobile_id:
            
            # seems to be looking for the most recent version of the settings blob
            uVar1 = None
            if is_big_block:
                uVar1 = struct.unpack("<I", spr[spr_offset+0x3:spr_offset+0x7])[0] & 0x00FFFFFF
            else:
                uVar1 = struct.unpack("<I", spr[spr_offset+0x2:spr_offset+0x6])[0] & 0x00FFFFFF

            # because, if we found a newer version, we use that instead
            if local_1c < uVar1:
                local_10 = i
                local_1c = uVar1

            # grab size and advance past it (in terms of pages)
            uVar2 = struct.unpack("<H", spr[spr_offset+0x07:spr_offset+0x09])[0]
            if (uVar2 >> 9) < 4:
                i += 4
            else:
                if (uVar2 & 0x01FF) != 0:
                    i += 1
                i += (uVar2 >> 9)
        else:
            i += 4

    return local_10

def find_and_dump_mobiles(max_pages: int,
                          is_big_block: bool,
                          spr: bytes,
                          pag: bytes):

    # id 0x30 is fsroot by the way... but we're not dumping the filesystem in this tool.
    #
    # special types not covered here:
    # 0x30 - fsroot on small block systems
    # 0x2A - filesystem data on big blocks
    # 0x2C - fsroot on big block systems
    for i in range(0x31, 0x3F):
        block_offset = find_mobile_block(max_pages, i, is_big_block, spr)
        if block_offset == 0:
            continue

        spr_offset = block_offset * 0x10
        pag_offset = block_offset * 0x200
        mobile_size = struct.unpack("<H", spr[spr_offset+0x07:spr_offset+0x09])[0]

        filename = f"Mobile{i + 0x11:c}.dat"
        print(f"Dumping: {filename} found at page 0x{block_offset:04x}, size {mobile_size} (0x{mobile_size:02x}) bytes")

        with open(filename, "wb") as fout:
            fout.write(pag[pag_offset:pag_offset+mobile_size])

def main():
    if len(sys.argv) < 2:
        usage()
        return
    
    spr = None
    pag = None

    with open(sys.argv[1], "rb") as f:
        # grab filesize C-style
        f.seek(0, os.SEEK_END)
        nand_size = f.tell()
        f.seek(0, os.SEEK_SET)

        if (nand_size % 0x4200) != 0:
            print("\n\n** Input file is not the correct size.\n")
            return
        
        nand_blocks = int(nand_size / 0x4200)

        # cap nand size at 64mbytes
        if nand_blocks >= 0x1000:
            nand_blocks = 0x1000
            nand_size = 0x4200000

        # code assumes 32 pages per block
        spr_size = nand_blocks * 0x200
        pag_size = nand_blocks * 0x4000

        print(f"{APP_NAME}\n-----------------------------------")

        print(f"Input File  :\n {sys.argv[1]}")
        print(f"Total Blocks:\n {nand_blocks}")

        spr = bytearray([0 * spr_size])
        pag = bytearray([0 * pag_size])

        print("Reading data now....please wait")
        for i in range(nand_blocks * 32):
            pag[i * 0x200:(i+1) * 0x200] = f.read(0x200)
            spr[i * 0x10:(i+1) * 0x10]  = f.read(0x10)
        
        # end of "we need file open" code
    
    is_big_block = spr[0] == 0xFF and spr[1] == 0x00
    if is_big_block:
        print("big block spare detected!")

    find_and_dump_mobiles(nand_blocks * 32,
                          is_big_block,
                          spr,
                          pag)

    print(f"\n{APP_NAME} Finished.")

if __name__ == "__main__":
    main()
