Import("env")

import tasmotapiolib
import os
import shutil
from os.path import join
import pathlib

# Install missed package
try:
    import esp_idf_size
except ImportError:
    env.Execute("$PYTHONEXE -m pip install esp-idf-size")


def firm_metrics(source, target, env):
    if env["PIOPLATFORM"] == "espressif32":
        firmware_file = tasmotapiolib.get_source_map_path(env).resolve()
        prev_firmware_file = firmware_file.parent.parent.with_name(
            env["PIOENV"] + "-firmware.map.old"
        )
        print("Archiving previous map in " + str(prev_firmware_file))
        report_file = (
            pathlib.Path(firmware_file).parent.resolve() / "firmware_metrics.json"
        )
        text_file = (
            pathlib.Path(firmware_file).parent.resolve() / "firmware_metrics.txt"
        )
        env.Execute(
            '$PYTHONEXE -m esp_idf_size --format=json2 "'
            + str(firmware_file)
            + '" -o "'
            + str(report_file)
            + '"'
        )
        if prev_firmware_file.exists():
            env.Execute(
                '$PYTHONEXE -m esp_idf_size --files --diff "'
                + str(prev_firmware_file)
                + '" --format=text "'
                + str(firmware_file)
                + '" -o "'
                + str(text_file)
                + '"'
            )
        else:
            env.Execute(
                '$PYTHONEXE -m esp_idf_size --files --format=text "'
                + str(firmware_file)
                + '" -o "'
                + str(text_file)
                + '"'
            )
        if prev_firmware_file.exists():
            os.remove(str(prev_firmware_file))
        shutil.copy(str(firmware_file), str(prev_firmware_file))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", firm_metrics)
