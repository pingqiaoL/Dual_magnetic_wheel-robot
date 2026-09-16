#!/usr/bin/env python3

import argparse
from pathlib import Path


def write_c_array(image: bytes, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("/* Generated from startup/etc by tools/generate_romfs.py. */\n\n")
        stream.write("const unsigned char romfs_img[] =\n{\n")
        for offset in range(0, len(image), 12):
            values = ", ".join(f"0x{value:02x}" for value in image[offset : offset + 12])
            stream.write(f"  {values},\n")
        stream.write("};\n\n")
        stream.write(f"const unsigned int romfs_img_len = {len(image)};\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    write_c_array(args.image.read_bytes(), args.output)


if __name__ == "__main__":
    main()
