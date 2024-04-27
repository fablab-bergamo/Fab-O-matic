Import("env")

import tasmotapiolib
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
        report_file = (
            pathlib.Path(firmware_file).parent.resolve() / "firmware_metrics.json"
        )
        env.Execute(
            '$PYTHONEXE -m esp_idf_size --format=json "'
            + str(firmware_file)
            + '" >"'
            + str(report_file)
            + '"'
        )
        env.Execute(
            '$PYTHONEXE -m esp_idf_size --format=text "' + str(firmware_file) + '"'
        )


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", firm_metrics)
