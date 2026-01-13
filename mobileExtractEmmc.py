'''
A brand new script for eMMC NANDs
'''

import os
import sys
import struct
import hashlib

ANCHOR_OFFSET_A = 0x02FE8000
ANCHOR_OFFSET_B = 0x02FEC000

def _emmc_validate_anchor(anchor):
    shadigest = hashlib.sha1(anchor[0x014:0x200]).digest()
    return shadigest == anchor[0x000:0x014]

def _pick_anchor(anchor_a, anchor_b):
    anchor_a_valid = _emmc_validate_anchor(anchor_a)
    anchor_b_valid = _emmc_validate_anchor(anchor_b)

    if anchor_a_valid and anchor_b_valid:
        anchor_a_version = struct.unpack(">H", anchor_a[0x1A:0x1C])[0]
        anchor_b_version = struct.unpack(">H", anchor_b[0x1A:0x1C])[0]
        return anchor_a if anchor_a_version > anchor_b_version else anchor_b
    
    print("warning: one or more anchors invalid; trying to pick winner...")

    if anchor_a_valid:
        print("anchor a valid; using that.")
        return anchor_a

    if anchor_b_valid:
        print("anchor b valid; using that.")
        return anchor_b

    return None

def _read_anchor_block(f, offset):
    f.seek(offset, os.SEEK_SET)
    return f.read(0x200)

def main():
    if len(sys.argv) < 2:
        print("please pass path to nand dump...")
        return
    
    with open(sys.argv[1], "rb") as f:
        # should be 0x03000000 for system partition
        # or 4 GB exactly for full NAND dump
        f.seek(0, os.SEEK_END)
        nand_size = f.tell()
        f.seek(0, os.SEEK_SET)

        if nand_size != 0x03000000 and nand_size != 0x01_00000000:
            print("error: invalid NAND dump size")
            return

        magic = f.read(2)
        if magic != bytes([0xFF, 0x4F]):
            print(f"error: magic word at start of file not ff 4f (got: {magic})")
            return
        
        print("this looks like a valid eMMC NAND, reading anchor blocks")
        
        anchor_a = _read_anchor_block(f, ANCHOR_OFFSET_A)
        anchor_b = _read_anchor_block(f, ANCHOR_OFFSET_B)

        anchor = _pick_anchor(anchor_a, anchor_b)
        if anchor is None:
            print("error: both anchor blocks invalid/corrupt, aborting.")
            return
        
        # entry at index 0 is fsroot, which we will not be parsing here
        for i in range(1, 10):
            offset = 0x1C + (i*4)
            mobile_location_16kblock = struct.unpack(">H", anchor[offset:offset+2])[0]
            if mobile_location_16kblock == 0:
                break

            mobile_size_bytes = struct.unpack(">H", anchor[offset+2:offset+4])[0]
            if mobile_size_bytes == 0:
                # presumably can't exceed this size, because eMMC uses 16k blocks
                mobile_size_bytes = 0x4000

            filename = f"Mobile{i + 0x41:c}.dat"
            print(f"Dumping: {filename} found at block 0x{mobile_location_16kblock:04x}, size {mobile_size_bytes} (0x{mobile_size_bytes:02x}) bytes")

            with open(filename, "wb") as fout:
                f.seek(mobile_location_16kblock * 0x4000, os.SEEK_SET)
                data = f.read(mobile_size_bytes)
                fout.write(data)

        print("all done here boss!!")

if __name__ == "__main__":
    main()
