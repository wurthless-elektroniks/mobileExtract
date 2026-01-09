# mobileExtract.exe reverse-engineered

Ancient Xbox 360 tool from fbBuild circa 2010, reverse engineered.
The program was reverse engineered in an attempt to find interesting snippets of code buried in the .exe.

fbBuild, by default, did not build NAND images with the system settings intact, so mobileExtract was created
to extract the MobileX.dat "files" containing the system settings. In reality, the system settings are not stored
as files in the NAND FlashFS, they are special pages floating around in the NAND.

There's not much reason to use this tool nowadays because it's long since been obsoleted by newer tools (I think).
It is buggy as hell, doesn't handle bad blocks correctly, and doesn't support eMMC dumps.

- pseudocode.c is a transcription/decompilation of the original code to pseudo-C. Don't blame me for the amazingly
  shitty code, it's how it was written (and compiler-optimized). But I've still tried to make it more readable.

- mobileExtract.py is a rewrite of the code in Python which makes it a lot cleaner, and more importantly allows it
  to be cross-platform.

About correctness: I don't know if this is 100% accurate yet. Might have to run this against the original tool
on Windows and also to see if I can grab the MobileX.dat files off real hardware (using a filemanager like Aurora)...
