import subprocess
import pkg_resources

Import("env")


def get_firmware_specifier_build_flag():
    ret = subprocess.run(
        ["git", "describe", "--tags", "--always"], stdout=subprocess.PIPE, text=True
    )  # Uses only annotated tags
    build_version = ret.stdout.strip()
    build_flag = '-D GIT_VERSION=\\"' + build_version + '\\"'
    print("Firmware Revision (GIT_VERSION) for the build: " + build_version)
    return build_flag


env.Append(BUILD_SRC_FLAGS=[get_firmware_specifier_build_flag()])
