import os
from os.path import join
import sys

Import("env")

def obj_to_dict(obj):
    return {k: getattr(obj, k, None) for k in dir(obj) if not k.startswith('_')}

def generate_factory_bin(source, target, env):
    # Retrieve crucial environment configuration variables
    platform = env.PioPlatform()
    board_config = env.BoardConfig()
    
    # Locate esptool path inside platform packages
    esptool_path = join(platform.get_package_dir("tool-esptoolpy") or "", "esptool.py")
    if not os.path.exists(esptool_path):
        print("[-] Error: esptool.py not found in package directory.")
        return

    # Extract target firmware paths
    build_dir = env.subst("$BUILD_DIR")
    firmware_factory = join(build_dir, "firmware_factory.bin")
    
    flash_size = board_config.get("upload.flash_size", "16MB")
    flash_freq = board_config.get("build.f_flash", "80m").replace("000000L", "m")
    flash_mode = board_config.get("build.flash_mode", "qio")

    # Read flash mapping offsets from the build environment
    flash_images = env.get("FLASH_EXTRA_IMAGES", [])
    
    # Build command arguments for esptool merge_bin
    cmd_args = [
        sys.executable, esptool_path,
        "--chip", "esp32p4",
        "merge_bin",
        "-o", firmware_factory,
        "--flash_mode", flash_mode,
        "--flash_size", flash_size,
        "--flash_freq", flash_freq
    ]

    # Map the primary application binary offset (usually 0x10000)
    app_offset = env.subst("$ESP32_APP_OFFSET") or "0x10000"
    app_bin = join(build_dir, "firmware.bin")
    
    # Append the bootloader, partition table, and extra images
    for offset, image in flash_images:
        cmd_args.extend([env.subst(offset), env.subst(image)])
        
    cmd_args.extend([app_offset, app_bin])

    print(f"[+] Merging binaries into structural factory image...")
    # Execute the esptool binary merger
    env.Execute(" ".join(cmd_args))

# Register the hook to trigger automatically post-compilation
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", generate_factory_bin)
